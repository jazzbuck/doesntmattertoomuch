#include "plugin.hpp"
#include <nanovg.h>
#include <stb_image.h>
#include <widget/TransparentWidget.hpp>
#include <dsp/digital.hpp>
#include <osdialog.h>

struct ImageData
{
    ImageData() = default;

    explicit ImageData(char const* file_path)
    {
        unsigned char* ptr = stbi_load(file_path, &width_, &height_, &comp_, 4);

        if (ptr == nullptr)
        {
            throw std::runtime_error("STBI failed to load in the image");
        }

        data_ = std::shared_ptr<unsigned char>{ptr, &stbi_image_free};
    }

    unsigned char const* data() const
    {
        return data_.get();
    }
    int height() const
    {
        return height_;
    }
    int width() const
    {
        return width_;
    }
    int comp() const
    {
        return comp_;
    }

private:
    std::shared_ptr<unsigned char> data_{};
    int height_{};
    int width_{};
    int comp_{};
};

struct PictureThis : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		CV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	PictureThis() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	}

	void process(const ProcessArgs& args) override
    {
        auto const comp = image_data_.comp();

        auto const num_channels = inputs[CLOCK_INPUT].getChannels();

        std::vector<float> voltage_per_channel(comp);
        if (num_channels < comp)
        {
            auto const input_voltage = inputs[CLOCK_INPUT].getVoltage();
            for (float& v : voltage_per_channel)
                v = input_voltage;
        }
        else
        {
            for (auto i = 0; i < comp; ++i)
            {
                voltage_per_channel[i] = inputs[CLOCK_INPUT].getVoltage(i);
            }
        }
        
        outputs[0].setChannels(comp);

        for (auto i = 0; i < comp; ++i)
        {
            bool const did_turn_on = trigger_per_channel_[i].process(voltage_per_channel[i]);
            if (did_turn_on)
            {
                auto& count = count_per_channel_[i];

                count = (count + 1) % (image_data_.width() * image_data_.height());

                int const image_index = i + comp * count;
                unsigned char const pixel_value = image_data_.data()[image_index];
                float const output_voltage = 10.0f * pixel_value / 255.0f;
                outputs[0].setVoltage(output_voltage, i);
                // std::cout << "Pixel value (" << i << ") = " << output_voltage << '\n';
            }
        }
	}

    void setImageData(ImageData data)
    {
        image_data_ = std::move(data);
        trigger_per_channel_.resize(image_data_.comp());
        count_per_channel_.resize(image_data_.comp(), 0);
    }

private:
    std::vector<dsp::SchmittTrigger> trigger_per_channel_{};
    std::vector<int> count_per_channel_{};
    ImageData image_data_;
};

struct PictureThisWidget;

struct LoadImageItem : MenuItem
{
    PictureThisWidget* widget_;

    void onAction(event::Action const&) override;
};

struct PngWidget : TransparentWidget
{
    explicit PngWidget(PictureThis* module)
        : module_{module}
    {}

    void setImage(std::string new_path)
    {
        new_image_ = true;
        image_data_ = ImageData(new_path.c_str());

        if (module_)
        {
            module_->setImageData(image_data_);
        }
    }

    void draw(DrawArgs const& args) override
    {
        if (!image_data_.data())
        {
            return;
        }

        if (new_image_)
        {
            nvg_handle_ = nvgCreateImageRGBA(args.vg, image_data_.width(), image_data_.height(), 0, image_data_.data());
            new_image_ = false;
        }

        nvgBeginPath(args.vg);

        auto const scale_factor_x = box.size.x / image_data_.width();
        auto const scale_factor_y = box.size.y / image_data_.height();
        auto const scale_factor = std::max(scale_factor_x, scale_factor_y);
        nvgScale(args.vg, scale_factor, scale_factor);

        NVGpaint const paint =
            nvgImagePattern(args.vg, x_, y_, image_data_.width(), image_data_.height(), 0.0f, nvg_handle_, 1.0f);
        nvgRect(args.vg, x_, y_, image_data_.width(), image_data_.height());
        nvgFillPaint(args.vg, paint);
        nvgFill(args.vg);

        nvgClosePath(args.vg);
    }

private:
    bool new_image_{};

    ImageData image_data_{};
    int x_{};
    int y_{};
    int nvg_handle_{};

    PictureThis* module_;
};

struct PictureThisWidget : ModuleWidget {
	PictureThisWidget(PictureThis* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PictureThis.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(16.576, 47.217)), module, PictureThis::CLOCK_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(16.576, 111.929)), module, PictureThis::CV_OUTPUT));

		// mm2px(Vec(116.851, 116.851))
		addChild(createWidget<Widget>(mm2px(Vec(29.864, 5.502))));

        // Add a demo image
        image_widget_ = new PngWidget(module);
        image_widget_->box.pos = Vec{100.0f, 50.0f};
        image_widget_->box.size = Vec{box.size.x - 150.0f, box.size.y - 100.0f};
        image_widget_->setImage(asset::plugin(pluginInstance, "res/Test.png"));
        addChild(image_widget_);
    }

    void appendContextMenu(Menu* menu) override
    {
        LoadImageItem* load_item = createMenuItem<LoadImageItem>("Load image (PNG)");
        load_item->widget_ = this;
        menu->addChild(load_item);
    }

    void loadImageDialog()
    {
        char* path_cstr = osdialog_file(OSDIALOG_OPEN, ".", nullptr, nullptr);
        if (!path_cstr)
        {
            return;
        }

        image_widget_->setImage(path_cstr);
        std::free(path_cstr);
    }

private:
    PngWidget* image_widget_;
};

void LoadImageItem::onAction(event::Action const& e)
{
    widget_->loadImageDialog();
}

Model* modelPictureThis = createModel<PictureThis, PictureThisWidget>("PictureThis");
