
#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#include "AudioSettings.h"
#include "Synth.h"

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 

class MainContentComponent : public Component
{
private:
	MidiKeyboardState keyboardState;
	MidiKeyboardComponent keyboardComponent;

	SharedResourcePointer<AudioSettings> sharedAudioSettings;
	AudioDeviceManager& inputDeviceManager = sharedAudioSettings->getInputDeviceManager();
	AudioDeviceManager& outputDeviceManager = sharedAudioSettings->getOutputDeviceManager();
	PiSynthPlayer synthPlayer;

public:
    MainContentComponent()
		: synthPlayer(keyboardState, inputDeviceManager, outputDeviceManager)
		, keyboardComponent(keyboardState, MidiKeyboardComponent::horizontalKeyboard)
	{
		setSize(1024, 300);
		setOpaque(true);

		addAndMakeVisible(keyboardComponent);

		keyboardComponent.setAvailableRange(21, 108);
	}

	void paint(Graphics& g) override
	{
		g.fillAll(Colours::black);
	}

	void resized() override
	{
		int w = (int)keyboardComponent.getKeyWidth() * (7 * 7 + 3), h = 50;
		keyboardComponent.setSize(w, h);
		keyboardComponent.setCentrePosition(getWidth() / 2, getHeight() / 2);
	}

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainContentComponent)
};

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 

// (This function is called by the app startup code to create our main component)
Component* createMainContentComponent()  {
	return new MainContentComponent(); 
}


#endif  // MAINCOMPONENT_H_INCLUDED
