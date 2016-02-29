#include "../JuceLibraryCode/JuceHeader.h"

Component* createMainContentComponent();

class PianoApplication : public JUCEApplication
{
public:
	PianoApplication() {}

	const String getApplicationName() override { return ProjectInfo::projectName; }
	const String getApplicationVersion() override { return ProjectInfo::versionString; }
	bool moreThanOneInstanceAllowed() override { return true; }

	void initialise(const String& /*commandLine*/) override {
		mainWindow = new MainWindow(getApplicationName());
	}

	void shutdown() override {
		mainWindow = nullptr;
	}

	void systemRequestedQuit() override {
		// This is called when the app is being asked to quit: you can ignore this
		// request and let the app carry on running, or call quit() to allow the app to close.
		quit();
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - 

	class MainWindow : public DocumentWindow
	{
	public:
		MainWindow(String name) : DocumentWindow(name,
			Colours::lightgrey,
			DocumentWindow::allButtons)
		{
			setUsingNativeTitleBar(true);
			setContentOwned(createMainContentComponent(), true);
			setResizable(true, true);

			centreWithSize(getWidth(), getHeight());
			setVisible(true);
		}

		void closeButtonPressed() override {
			JUCEApplication::getInstance()->systemRequestedQuit();
		}

		/* Note: Be careful if you override any DocumentWindow methods - the base
		class uses a lot of them, so by overriding you might break its functionality.
		It's best to do all your work in your content component instead, but if
		you really have to override any DocumentWindow methods, make sure your
		subclass also calls the superclass's method.
		*/

	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
	};

private:
	ScopedPointer<MainWindow> mainWindow;
};

// generates the main() routine that launches the app
START_JUCE_APPLICATION(PianoApplication)
