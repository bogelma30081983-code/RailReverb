#include "PluginProcessor.h"
#include "PluginEditor.h"

// --- Реалізація RailProfileComponent ---
void RailProfileComponent::setRailSize(float newSize) { railSize = std::clamp(newSize, 0.0f, 1.0f); repaint(); }

void RailProfileComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat().reduced(10.0f);
    g.fillAll(juce::Colours::black.withAlpha(0.2f));
    g.setColour(juce::Colours::orange.withAlpha(0.1f));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 6.0f, 1.0f);

    g.setColour(juce::Colours::orange.withAlpha(0.04f));
    for (int i = 0; i < getWidth(); i += 15) g.drawVerticalLine(i, 0.0f, (float)getHeight());
    for (int i = 0; i < getHeight(); i += 15) g.drawHorizontalLine(i, 0.0f, (float)getWidth());

    float baseWidth = bounds.getWidth() * (0.50f + (railSize * 0.35f));
    float headWidth = bounds.getWidth() * (0.30f + (railSize * 0.25f));
    float neckWidth = bounds.getWidth() * (0.08f + (railSize * 0.07f));
    float totalHeight = bounds.getHeight() * 0.85f;
    float centerX = bounds.getCentreX(), bottomY = bounds.getBottom() - 5.0f, topY = bottomY - totalHeight;

    juce::Path p;
    p.startNewSubPath(centerX - baseWidth * 0.5f, bottomY);
    p.lineTo(centerX - baseWidth * 0.5f, bottomY - 6.0f);
    p.quadraticTo(centerX - neckWidth * 1.5f, bottomY - 12.0f, centerX - 
        neckWidth * 0.5f, bottomY - totalHeight * 0.3f);
    p.lineTo(centerX - neckWidth * 0.5f, topY + totalHeight * 0.25f);
    p.quadraticTo(centerX - headWidth * 0.5f, topY + totalHeight * 0.15f, centerX - headWidth * 0.5f, topY + 8.0f);
    p.quadraticTo(centerX - headWidth * 0.5f, topY, centerX - headWidth * 0.3f, topY);
    p.lineTo(centerX + headWidth * 0.3f, topY);
    p.quadraticTo(centerX + headWidth * 0.5f, topY, centerX + headWidth * 0.5f, topY + 8.0f);
    p.quadraticTo(centerX + headWidth * 0.5f, topY + totalHeight * 0.15f, centerX + neckWidth * 0.5f, topY + totalHeight * 0.25f);
    p.lineTo(centerX + neckWidth * 0.5f, bottomY - totalHeight * 0.3f);
    p.quadraticTo(centerX + neckWidth * 1.5f, bottomY - 12.0f, centerX + baseWidth * 0.5f, bottomY - 6.0f);
    p.lineTo(centerX + baseWidth * 0.5f, bottomY);
    p.closeSubPath();

    juce::ColourGradient grad(juce::Colours::grey.brighter(),
        centerX - headWidth, topY, juce::Colours::darkgrey.darker(), centerX + baseWidth, bottomY, false);
    g.setGradientFill(grad); g.fillPath(p);
    g.setColour(juce::Colours::orange.withAlpha(0.8f)); g.strokePath(p, juce::PathStrokeType(1.5f));
}

// --- Реалізація RailWindVisualizer ---
RailWindVisualizer::RailWindVisualizer(RailReverbProcessor& p) : processor(p) { startTimerHz(60); }
RailWindVisualizer::~RailWindVisualizer() { stopTimer(); }

void RailWindVisualizer::timerCallback() {
    auto [l, r] = processor.getTailLevels();
    float level = (std::abs(l) + std::abs(r)) * 0.5f;
    windIntensity = (level > windIntensity) ? level : windIntensity * 0.94f;
    repaint();
}

void RailWindVisualizer::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::black.withAlpha(0.5f));
    g.setColour(juce::Colours::orange.withAlpha(0.1f)); g.drawRect(getLocalBounds(), 1);
    auto bounds = getLocalBounds().toFloat();
    float midY = bounds.getCentreY(), w = bounds.getWidth(), h = bounds.getHeight();

    g.setColour(juce::Colours::green.withAlpha(std::clamp(windIntensity * 2.5f, 0.0f, 0.75f)));
    juce::Path p; p.startNewSubPath(0, midY);
    float time = (float)juce::Time::getMillisecondCounterHiRes() * 0.004f;
    for (float x = 0; x < w; x += 3.0f) {
        float y = midY + ((std::sin(x * 0.02f + time) 
            + std::cos(x * 0.04f - time * 0.7f)) * windIntensity * (h * 0.525f));
        p.lineTo(x, y);
    }
    g.strokePath(p, juce::PathStrokeType(2.0f));

    if (windIntensity > 0.02f) {
        g.setColour(juce::Colours::orange.withAlpha(std::clamp(windIntensity * 3.5f, 0.3f, 0.95f)));
        int dots = std::clamp(static_cast<int>(windIntensity * 150.0f), 0, 30);
        for (int i = 0; i < dots; ++i)
            g.fillEllipse(rand.nextFloat() * w, midY + 
                (rand.nextFloat() - 0.5f) * (h * 3.0f) * 
                windIntensity, rand.nextFloat() * 3.0f + 1.0f, rand.nextFloat() * 3.0f + 1.0f);
    }
    g.setColour(juce::Colours::whitesmoke.withAlpha(0.3f)); g.setFont(10.0f);
    g.drawText("RAIL WIND: " + 
        juce::String(juce::Decibels::gainToDecibels(windIntensity + 0.00001f), 1) 
        + " dB", 10, h - 18, 200, 12, juce::Justification::left);
}

// --- ОДРАЗУ НИЖЧЕ у вашому файлі вже йде поточний конструктор Едітора: ---
// RailReverbEditor::RailReverbEditor (RailReverbProcessor& p) ...

// Конструктор графічного інтерфейсу
RailReverbEditor::RailReverbEditor(RailReverbProcessor& p)
    : AudioProcessorEditor(&p), processor(p), visualizer(p)
{
    // Робимо широке вікно для креслення рейки збоку
    setSize(680, 450);
    setResizable(true, true);
    setResizeLimits(480, 300, 680, 450);
    // Застосовуємо кастомний дизайн до крутілок
    detuneSlider.setLookAndFeel(&railLookAndFeel);
    
    cutoffSlider.setLookAndFeel(&railLookAndFeel);
    mixSlider.setLookAndFeel(&railLookAndFeel);
    sizeSlider.setLookAndFeel(&railLookAndFeel); // Слайдер розміру теж зміниться!

    // Застосовуємо кастомний дизайн до кнопок-чекбоксів
    driftButton.setLookAndFeel(&railLookAndFeel);
    noiseButton.setLookAndFeel(&railLookAndFeel);
    compButton.setLookAndFeel(&railLookAndFeel);
    materialBox.setLookAndFeel(&railLookAndFeel);

    // Слайдер зміни об'єму рейки
    sizeSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    sizeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    sizeSlider.addListener(this);
    addAndMakeVisible(sizeSlider);
    sizeAttachment = 
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts,
            "SIZE", sizeSlider);

    // Візуальний профіль заліза
    addAndMakeVisible(railProfile);
    railProfile.setRailSize(processor.apvts.getRawParameterValue("SIZE")->load());

    // Випадаючий список матеріалів
    materialBox.addItemList({ "Iron (Steel)", "Copper", "Brass", "Silver", "Gold", "Tungsten", 
        "Titanium", "Bog Oak", "Uranium" }, 1);
    addAndMakeVisible(materialBox);
    materialAttachment = 
        std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.apvts, 
            "MATERIAL", materialBox);

    // Кнопка аналогового дріфту
    driftButton.setButtonText("Analog Drift");
    driftButton.setClickingTogglesState(true);
    addAndMakeVisible(driftButton);
    driftAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, "DRIFT", driftButton);

    // Кнопка аналогового шуму
    noiseButton.setButtonText("Analog Noise");
    noiseButton.setClickingTogglesState(true);
    addAndMakeVisible(noiseButton);
    noiseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts, "NOISE", noiseButton);

    // Створення крутілок (Детюн, Фільтр, Мікс)
    setupRotary(detuneSlider, "DETUNE");
    setupRotary(cutoffSlider, "CUTOFF");
    setupRotary(mixSlider, "MIX");

    // Кнопка розумної компресії
    compButton.setButtonText("Auto Gain Comp");
    compButton.setClickingTogglesState(true);
    addAndMakeVisible(compButton);
    compAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
        (processor.apvts, "AUTO_COMP", compButton);

    // Візуалізатор хвоста
    addAndMakeVisible(visualizer);
}

RailReverbEditor::~RailReverbEditor()
{
    sizeSlider.removeListener(this);
}

// Колбек на зміну слайдера для деформації малюнка
void RailReverbEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &sizeSlider)
    {
        railProfile.setRailSize(static_cast<float>(sizeSlider.getValue()));
    }
}

// Малювання фону та написів
void RailReverbEditor::paint(juce::Graphics& g)
{
    //g.fillAll(juce::Colours::darkgrey.darker().darker());

    // Плавний градієнт від темно-сірого до майже чорного
    juce::ColourGradient gradient(juce::Colour(0xff2c3035), 0.0f, 0.0f,
        juce::Colour(0xff0f1112), 0.0f, (float)getHeight(), false);
    g.setGradientFill(gradient);
    g.fillAll();

    g.setColour(juce::Colours::orange);
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    g.drawText("RAIL REVERB MODELER", 20, 20, 400, 30, juce::Justification::left);

    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(12.0f);
    g.drawText("Tram Rail (Min)", 20, 95, 100, 20, juce::Justification::left);
    g.drawText("Heavy Train Rail (Max)", 310, 95, 150, 20, juce::Justification::right);

    g.setColour(juce::Colours::orange.withAlpha(0.6f));
    g.drawText("PHYSICAL PROFILE", 480, 45, 180, 20, juce::Justification::centred);

    // --- НАПИСИ НАД КРУТІЛКАМИ ---
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(13.0f, juce::Font::plain)); // Валідний синтаксис для JUCE 8

    // Малюємо текст автоматично на 20 пікселів вище за кожну крутілку
    g.drawText("DETUNE", detuneSlider.getX(), detuneSlider.getY() - 20, detuneSlider.getWidth(), 20, juce::Justification::centred);
    g.drawText("CUTOFF", cutoffSlider.getX(), cutoffSlider.getY() - 20, cutoffSlider.getWidth(), 20, juce::Justification::centred);
    g.drawText("MIX", mixSlider.getX(), mixSlider.getY() - 20, mixSlider.getWidth(), 20, juce::Justification::centred);
}

// Лейаут (координати) елементів інтерфейсу
void RailReverbEditor::resized()
{
    sizeSlider.setBounds(20, 70, 440, 25);
    railProfile.setBounds(480, 70, 180, 350); // Креслення рейки справа збоку

    materialBox.setBounds(20, 130, 180, 30);
    driftButton.setBounds(210, 130, 110, 30);
    noiseButton.setBounds(330, 130, 110, 30);

    detuneSlider.setBounds(20, 190, 100, 100);
    cutoffSlider.setBounds(135, 190, 100, 100);
    mixSlider.setBounds(250, 190, 100, 100);

    compButton.setBounds(365, 225, 105, 30);
    visualizer.setBounds(20, 310, 440, 110);
}

// Хелпер ініціалізації крутілок
void RailReverbEditor::setupRotary(juce::Slider& s, const juce::String& pid)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    addAndMakeVisible(s);

    if (pid == "DETUNE") detuneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, pid, s);
    if (pid == "CUTOFF") cutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, pid, s);
    if (pid == "MIX") mixAttachment = 
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts, pid, s);
}