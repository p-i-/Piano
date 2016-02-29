// Settings.h

#ifndef SETTINGS_H_INCLUDED
#define SETTINGS_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

class AudioSettings : ChangeListener
{
private:
	ScopedPointer<AudioDeviceManager> pInputDeviceManager;
	ScopedPointer<AudioDeviceManager> pOutputDeviceManager;

	void launch(AudioDeviceManager* pDeviceManager, int inputs, int outputs)
	{
		AudioDeviceSelectorComponent* settingsPane = new AudioDeviceSelectorComponent(
			*pDeviceManager,
			inputs, inputs,		// min/max inputs
			outputs, outputs,	// min/max outputs
			inputs > 0,			// showMidiInputOptions
			outputs > 0,		// showMidiOutputSelector
			true,				// showChannelsAsStereoPairs
			true				// showAdvancedOptions
			);

		settingsPane->setSize(640, 480);


		DialogWindow::LaunchOptions options;

		options.content.setOwned(settingsPane);
		options.dialogTitle = "Audio Settings";
		options.dialogBackgroundColour = Colours::lightgrey;
		options.escapeKeyTriggersCloseButton = true;
		options.useNativeTitleBar = true;
		options.resizable = true;

		DialogWindow* dialogWindow = options.launchAsync();
		dialogWindow->centreWithSize(450, 250);
	}

public:
	AudioDeviceManager& getInputDeviceManager() {
		return *pInputDeviceManager;
	}
	AudioDeviceManager& getOutputDeviceManager() {
		return *pOutputDeviceManager;
	}

	AudioSettings()
	{
		pInputDeviceManager = new AudioDeviceManager();
		pOutputDeviceManager = new AudioDeviceManager();

		pInputDeviceManager->initialise(
			0,			// numInputChannelsNeeded
			0,			// numOutputChannelsNeeded
			nullptr,	// XML
			true,		// selectDefaultDeviceOnFailure
			String(),	// preferredDefaultDeviceName
			0			// preferredSetupOptions
			);

		pOutputDeviceManager->initialise(
			0,			// numInputChannelsNeeded
			1,			// numOutputChannelsNeeded
			nullptr,	// XML
			true,		// selectDefaultDeviceOnFailure
			String(),	// preferredDefaultDeviceName
			0			// preferredSetupOptions
			);

		pOutputDeviceManager->addChangeListener(this);

		DBG("AudioIO!");

		OwnedArray<AudioIODeviceType> types;
		pOutputDeviceManager->createAudioDeviceTypes(types);
		for (auto& t : types) {
			String typeName(t->getTypeName());				// This will be things like "DirectSound", "CoreAudio", etc.
			DBG(typeName);
			t->scanForDevices();							// This must be called before getting the list of devices
			StringArray deviceNames(t->getDeviceNames());	// This will now return a list of available devices of this type
			for (auto& d : deviceNames)
				DBG("   " + d);
			//AudioIODevice* device = t->createDevice(deviceNames[j], String());
		}
	}

	void changeListenerCallback(ChangeBroadcaster*) override
	{
		dumpDeviceInfo();
	}

	void dumpDeviceInfo()
	{
		AudioDeviceManager& dm = *pOutputDeviceManager;

		DBG("--------------------------------------");
		DBG("Current audio device type: " + (dm.getCurrentDeviceTypeObject() != nullptr
			? dm.getCurrentDeviceTypeObject()->getTypeName()
			: "<none>"));

		if (AudioIODevice* device = dm.getCurrentAudioDevice())
		{
			DBG("   Current audio device: " + device->getName().quoted());
			//logMessage("Sample rate: " + String(device->getCurrentSampleRate()) + " Hz");
			//logMessage("Block size: " + String(device->getCurrentBufferSizeSamples()) + " samples");
			//logMessage("Output Latency: " + String(device->getOutputLatencyInSamples()) + " samples");
			//logMessage("Input Latency: " + String(device->getInputLatencyInSamples()) + " samples");
			//logMessage("Bit depth: " + String(device->getCurrentBitDepth()));
			//logMessage("Input channel names: " + device->getInputChannelNames().joinIntoString(", "));
			//logMessage("Active input channels: " + getListOfActiveBits(device->getActiveInputChannels()));
			//logMessage("Output channel names: " + device->getOutputChannelNames().joinIntoString(", "));
			//logMessage("Active output channels: " + getListOfActiveBits(device->getActiveOutputChannels()));
		}
		else
		{
			DBG("No audio device open");
		}
	}

	void launchInputWindow() {
		launch(pInputDeviceManager, 1, 0);
	}

	void launchOutputWindow() {
		launch(pOutputDeviceManager, 0, 2);
	}
};

#endif  // SETTINGS_H_INCLUDED
