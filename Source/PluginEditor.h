#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
//kastom
class RailLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RailLookAndFeel()
    {
        // Загальні кольори для тексту та елементів
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);

        // --- КАСТОМІЗАЦІЯ ВИПАДАЮЧОГО СПИСКУ (ComboBox) ---
        // Колір тексту на кнопці (коли список закритий)
        setColour(juce::ComboBox::textColourId, juce::Colour(0xfff7a400));
        // Колір стрілочки праворуч
        setColour(juce::ComboBox::arrowColourId, juce::Colour(0xfff7a400));
        // Колір рамки навколо кнопки
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff495057));

        // --- КАСТОМІЗАЦІЯ ВІДКРИТОГО МЕНЮ (PopupMenu) ---
        // Колір тексту елементів у списку
        setColour(juce::PopupMenu::textColourId, juce::Colour(0xfff7a400));
        // Колір фону самого віконця, що випадає (темна сталь)
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff212529));
        // Колір фону рядка, на який ти наводиш мишкою (підсвітка)
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xfff7a400));
        // Колір тексту, коли на елемент наведено мишку (робимо чорним для контрасту)
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    }

    // Малюємо круту металеву крутілку з помаранчевим треком
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
        juce::Slider& slider) override
    {
        auto radius = juce::jmin(width, height) / 2.0f - 6.0f;
        auto centreX = x + width * 0.5f;
        auto centreY = y + height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // 1. Задня підкладка крутілки (темна сталь)
        g.setColour(juce::Colour(0xff212529));
        g.fillEllipse(rx, ry, rw, rw);
        g.setColour(juce::Colour(0xff343a40));
        g.drawEllipse(rx, ry, rw, rw, 2.0f);

        // 2. Активний помаранчевий хвіст (дуга навколо)
        juce::Path arcPath;
        arcPath.addCentredArc(centreX, centreY, radius + 3.0f, radius + 3.0f, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(juce::Colour(0xfff7a400)); // Наш фірмовий помаранчевий
        g.strokePath(arcPath, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // 3. Лінія-вказівник (нотч) на самій крутілці
        juce::Path p;
        auto pointerLength = radius * 0.8f;
        auto pointerThickness = 3.0f;
        p.addRoundedRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength, 1.0f);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        g.setColour(juce::Colour(0xfff7a400));
        g.fillPath(p);
    }
    // Малюємо стильний кастомний горизонтальний трек і повзунок
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float minSliderPos, float maxSliderPos,
        juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        // Перевіряємо, що це саме горизонтальний слайдер
        if (style == juce::Slider::LinearHorizontal)
        {
            auto trackHeight = 6.0f;
            auto trackY = y + height * 0.5f - trackHeight * 0.5f;

            // 1. Задня підкладка рейки/треку (темна сталь)
            g.setColour(juce::Colour(0xff212529));
            g.fillRoundedRectangle(x, trackY, width, trackHeight, 3.0f);
            g.setColour(juce::Colour(0xff343a40));
            g.drawRoundedRectangle(x, trackY, width, trackHeight, 3.0f, 1.0f);

            // 2. Активна частина треку (помаранчевий неоновий колір до самого повзунка)
            g.setColour(juce::Colour(0xfff7a400));
            g.fillRoundedRectangle(x, trackY, sliderPos - x, trackHeight, 3.0f);

            // 3. Малюємо сам повзунок (важка металева "шпала")
            auto thumbWidth = 14.0f;
            auto thumbHeight = 22.0f;
            auto thumbX = sliderPos - thumbWidth * 0.5f;
            auto thumbY = y + height * 0.5f - thumbHeight * 0.5f;

            // Тіло повзунка (індустріальний сірий метал)
            g.setColour(juce::Colour(0xff495057));
            g.fillRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 3.0f);

            // Світла рамка повзунка для об'єму
            g.setColour(juce::Colour(0xff6c757d));
            g.drawRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 3.0f, 1.5f);

            // Вертикальна світлодіодна помаранчева лінія по центру повзунка
            g.setColour(juce::Colour(0xfff7a400));
            g.fillRect(sliderPos - 1.0f, thumbY + 4.0f, 2.0f, thumbHeight - 8.0f);
        }
    }


    // Малюємо стильні квадратні чекбокси замість стандартних круглих
    void drawTickBox(juce::Graphics& g, juce::Component& component,
        float x, float y, float w, float h,
        bool ticked, bool isEnabled, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto boxRegion = juce::Rectangle<float>(x, y, w, h).reduced(2.0f);

        // Фон віконця
        g.setColour(juce::Colour(0xff212529));
        g.fillRoundedRectangle(boxRegion, 3.0f);
        g.setColour(juce::Colour(0xff495057));
        g.drawRoundedRectangle(boxRegion, 3.0f, 1.5f);

        // Якщо галочка активна — малюємо помаранчевий квадрат всередині
        if (ticked)
        {
            g.setColour(juce::Colour(0xfff7a400));
            g.fillRoundedRectangle(boxRegion.reduced(3.0f), 2.0f);
        }
    }
};

//==========
class RailProfileComponent : public juce::Component
{
public:
    void setRailSize(float newSize);
    void paint(juce::Graphics& g) override;
private:
    float railSize = 0.5f;
};

class RailWindVisualizer : public juce::Component, public juce::Timer
{
public:
    RailWindVisualizer(RailReverbProcessor& p);
    ~RailWindVisualizer() override;
    void timerCallback() override;
    void paint(juce::Graphics& g) override;
private:
    RailReverbProcessor& processor;
    float windIntensity = 0.0f;
    juce::Random rand;
};

class RailReverbEditor : public juce::AudioProcessorEditor, public juce::Slider::Listener
{
public:
    RailReverbEditor(RailReverbProcessor&);
    ~RailReverbEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void sliderValueChanged(juce::Slider* slider) override;

private:
    void setupRotary(juce::Slider& s, const juce::String& pid);

    // Додай це в приватну секцію твого RailReverbEditor:
    RailLookAndFeel railLookAndFeel;

    RailReverbProcessor& processor;
    juce::Slider sizeSlider;
    RailProfileComponent railProfile;
    juce::ComboBox materialBox;
    juce::ToggleButton driftButton, noiseButton, compButton;
    juce::Slider detuneSlider, cutoffSlider, mixSlider;
    RailWindVisualizer visualizer;

    // ПРАВИЛЬНО:
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> 
        sizeAttachment, detuneAttachment, cutoffAttachment, mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> 
        materialAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
        driftAttachment, noiseAttachment, compAttachment;

   JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RailReverbEditor)
};