#include <JuceHeader.h>

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceGui;

    const juce::File pluginFile(argc > 1 ? argv[1]
                                         : (juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                                                .getChildFile("Library/Audio/Plug-Ins/VST3/Bifrost.vst3")
                                                .getFullPathName()));

    if (!pluginFile.exists())
    {
        std::cerr << "Plugin does not exist: " << pluginFile.getFullPathName() << "\n";
        return 2;
    }

    juce::AudioPluginFormatManager formatManager;
    formatManager.addFormat(std::make_unique<juce::VST3PluginFormat>());

    juce::OwnedArray<juce::PluginDescription> descriptions;
    for (auto* format : formatManager.getFormats())
        format->findAllTypesForFile(descriptions, pluginFile.getFullPathName());

    if (descriptions.isEmpty())
    {
        std::cerr << "No plugin descriptions found for " << pluginFile.getFullPathName() << "\n";
        return 3;
    }

    std::unique_ptr<juce::PluginDescription> selected;
    for (auto* description : descriptions)
    {
        std::cout << "Found: " << description->name << " | "
                  << description->manufacturerName << " | "
                  << description->pluginFormatName << " | "
                  << description->category << "\n";

        if (description->pluginFormatName == "VST3" && description->name == "Bifrost")
            selected.reset(new juce::PluginDescription(*description));
    }

    if (selected == nullptr)
    {
        std::cerr << "Bifrost VST3 description was not found\n";
        return 4;
    }

    juce::String error;
    std::unique_ptr<juce::AudioPluginInstance> instance(
        formatManager.createPluginInstance(*selected, 44100.0, 512, error));

    if (instance == nullptr)
    {
        std::cerr << "Instantiation failed: " << error << "\n";
        return 5;
    }

    instance->prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;
    instance->processBlock(buffer, midi);

    std::unique_ptr<juce::AudioProcessorEditor> editor(instance->createEditorIfNeeded());
    if (editor == nullptr && instance->hasEditor())
    {
        std::cerr << "Plugin reports an editor, but editor creation returned null\n";
        return 6;
    }

    instance->releaseResources();
    std::cout << "Plugin smoke load passed\n";
    return 0;
}
