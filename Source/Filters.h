// Algorithm from JOS & Jaffe 1983 http://pipad.org/src/spiral/Jaffe-Smith-Extensions-CMJ-1983.pdf
// Web version at: http://pipad.org/spiral.html

#ifndef FILTERS_H_INCLUDED
#define FILTERS_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#include <memory>
#include <algorithm>
#include <vector>

// - - - - - - - - - - - - - - - - - - - - - 
// filterA -- 2pt MOVING AVERAGE weighted by S, decay rho
//      y[n] = rho { S x[n] + (1-S) x[n-1] }        (Eqs. 18 & 20)
//
class WeightedAv {
	float S, oneMinusS, rho, A, B, x_prev = 0;
public:
	float out = 0;
	WeightedAv(float _S, float _rho) : S(_S), oneMinusS(1-_S), rho(_rho) {
		A = rho * S;
		B = rho * oneMinusS;
	}
	void input(float x) noexcept {
		out = A*x + B*x_prev;
		x_prev = x;
	}
};

// - - - - - - - - - - - - - - - - - - - - - 
// filterB -- N-step DELAY 
//      y[n] = x[n-N]
//
class Delay {
	float* buf;
	int ptr = 0, N;
public:
	float out = 0;
	Delay(int _N) : N(_N) {
		buf = (float*)calloc(N,sizeof(float));
	}
	~Delay() {
		free(buf);
	}
	void input(float x) noexcept {
		out = buf[ptr];
		buf[ptr] = x;
		ptr = (ptr + 1) % N;
	}
};

// - - - - - - - - - - - - - - - - - - - - - 
// filterC --  Fractional-delay TUNING filter (Eq.12) 
//      y[n] = C x[n] + x[n-1] - C y[n-1]
//
class FracDelay {
	float C, x_prev = 0, y_prev = 0;
public:
	float out = 0;
	FracDelay(float _C) : C(_C)
	{ }
	void input(float x) noexcept {
		out = C*x + x_prev - C*y_prev;
		x_prev = x;
		y_prev = out;
	}
};

// - - - - - - - - - - - - - - - - - - - - - 
// filter D -- balance fundamental amp over spectrum
//      y[n] = (1-R) x[n] + R y[n-1]
//
class BalanceFundAmp {
	float R, oneMinusR, y_prev = 0;
public:
	float out = 0;
	BalanceFundAmp(float _R) : R(_R), oneMinusR(1-_R)
	{ }
	void input(float x) noexcept {
		out = oneMinusR*x + R*y_prev;
		y_prev = out;
	}
};

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 

float lerp(float U0, float U1, float u, float V0, float V1) {
	float k = (u - U0) / (U1 - U0);
	return (1 - k)*V0 + k*V1;
}

class JOS {
	std::vector<float> x;
	int counter, /*counter_cutoff, - disused */ N;
	float env, energy, alpha;

	const float pi = 3.141592653589793238462643f,  tau = 2 * pi,
		S_MIN = .5f,
		S_MAX = .98f,
		t60_MIN = 15.f, // time in seconds to reach 1/1000 original volume for lowest ..
		t60_MAX = 9.f,  // .. and highest midi-note
		NOTE_OFF_ENV_DECAY = 0.9995f,
		// VOL_CUTOFF = 1 / 1000.f, // disused ..
		CUTOFF_ENERGY = .001f, // replacement!
		freq_A0 = 27.5f;

	ScopedPointer<BalanceFundAmp>	balanceFundAmp;
	ScopedPointer<FracDelay>		fracDelay;
	ScopedPointer<Delay>			delay;
	ScopedPointer<WeightedAv>		weightedAv;

	int midiNoteNumber;
public:
	JOS(int m, float vol, float fs)
		: midiNoteNumber(m)
	{
		// - - - - - - - - - - - - - - - - - - - - - 
		// filterA -- 2pt MOVING AVERAGE weighted by S, decay rho
		//      y[n] = rho { S x[n] + (1-S) x[n-1] }        (Eqs. 18 & 20)
		//
		float P, S, lambdaf, _alpha;
		{
			float f1 = (float)MidiMessage::getMidiNoteInHertz(midiNoteNumber);
			
			P = fs / f1; // samps for target fund. period

			float lambdaindex = lerp(21.f, 108.f, (float)midiNoteNumber, 0.f, 1.f);

			lambdaf = f1 / fs;

			S = lerp(0, 1, lambdaindex * lambdaindex, S_MIN, S_MAX);

			// Gain of filterA over one period of f1 (Eq. 21)
			float Ga_f1 = sqrt((1 - S)*(1 - S) + S*S + 2 * S*(1 - S)*cos(tau * lambdaf));

			// 1-samp-decay for filterA
			//   alphaA^P = Ga_f1   (Eq. 10)
			float alphaA = powf(Ga_f1, 1 / P);

			// t60 is time(sec) to reach 1/1000 vol
			float desired_t60 = lerp(0, 1, lambdaindex, t60_MIN, t60_MAX);

			// alpha is OVERALL per-sample decay required to give desired t60
			// alpha^(desired_t60 * fs) = .001
			_alpha = powf(.001f, 1 / (desired_t60*fs));

			// Calculate additional decay required
			// rho causes a per-sample decay of rho^lambdaf (just after Eq. 18)
			// so:
			//    alpha = alphaA * rho^lambdaf
			float rho = std::min(1.f, powf(_alpha / alphaA, 1 / lambdaf));

			weightedAv = new WeightedAv(S, rho);
		}
		alpha = _alpha;

		// - - - - - - - - - - - - - - - - - - - - - 
		// filterB -- N-step DELAY 
		//      y[n] = x[n-N]
		//

		float Pa = S,								// period of filterA, (Eq. 22)
			epsilon = 0.01f;						// choose in 0,1 close to 0 (Eq. 14-15)
			
		N = (int)floor(P - Pa - epsilon);			// (Eq. 15)

		delay = new Delay(N);


		// - - - - - - - - - - - - - - - - - - - - - 
		// filterC --  Fractional-delay TUNING filter (Eq.12) 
		//      y[n] = C x[n] + x[n-1] - C y[n-1]
		//
		{
			float Pc = P - N - Pa,               // (Eq. 15)
				C = (1 - Pc) / (1 + Pc);         // (Eq. 17)

			fracDelay = new FracDelay(C);
		}

		// - - - - - - - - - - - - - - - - - - - - - 
		// filter D -- balance fundamental amp over spectrum
		//      y[n] = (1-R) x[n] + R y[n-1]
		//
		{
			float index_C4 = 3 + 3 * 12,
				f_m = freq_A0 *powf(2.f, index_C4 / 12.f),
				// VOLUME
				// So if I had 10 volume steps, equally spaced on log-freq 
				// between 0 and Nyquist, these would be ι, ι^2, ..., ι^10 = nyquist
				nyquist = fs / 2,
				L = powf(nyquist, vol),
				lambdaL = L / nyquist,
				R_L = exp(-pi * lambdaL / 2),

				theta = -tau * f_m / fs,
				re = -R_L * cos(theta) + 1,
				im = -R_L * sin(theta),
				mag = sqrt(re*re + im*im),
				G_L = (1 - R_L) / mag,

				phi = -pi * lambdaf,
				u = sqrt(1 - powf(G_L*cos(phi), 2)),
				v = G_L*sin(phi),
				w = (u + v) / (u - v),
				R = std::min(w, 1 / w);

			balanceFundAmp = new BalanceFundAmp(R);
		}

		// - - - - - - - - - - - - - - - - - - - - - - - - - 

		// http://stackoverflow.com/questions/9878965/c-rand-between-0-and-1
		x = std::vector<float>(N + 1);
		for (int i = 0; i < N + 1; i++) 
			x[i] = 2 * rand() / (float)RAND_MAX - 1;

		// vol * alpha^counter_cutoff = VOL_CUTOFF => counter_cutoff ln alpha = log(VOL_CUTOFF / vol)
		counter = 0;
		//counter_cutoff = (int)( log(VOL_CUTOFF / vol) / log(alpha) ); /* disused */

		env = 1;
		energy = 1;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

	void renderToBuffer(float* outbuf, int samples, bool isDown) 
	{
		float env_decay = isDown ? 1 : NOTE_OFF_ENV_DECAY;

		for (int n = 0; n < samples; n++) {
			float x_n = (counter <= N) ? x[counter] : 0;

			balanceFundAmp->input(x_n);

			delay->input(balanceFundAmp->out + fracDelay->out);
			weightedAv->input(delay->out);
			fracDelay->input(weightedAv->out);

			outbuf[n] += delay->out * env;

			counter++;

			env    *=       env_decay;
			energy *= alpha*env_decay;
		}
	}

	bool is_alive() {
		return energy > CUTOFF_ENERGY;
	}

	float getEnergy() {
		return energy;
	}

	int getIndex() {
		return midiNoteNumber;
	}
};

#endif  // FILTERS_H_INCLUDED
