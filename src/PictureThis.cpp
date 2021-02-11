#include "plugin.hpp"
#include <nanovg.h>
#include <stb_image.h>
#include <widget/TransparentWidget.hpp>

struct ImageData
{
    ImageData() = default;

    explicit ImageData(char const* file_path)
    {
        data_ = std::shared_ptr<unsigned char>{stbi_load(file_path, &width_, &height_, &comp_, 4), &stbi_image_free};
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
		R_OUTPUT,
		G_OUTPUT,
		B_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	PictureThis() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	}

	void process(const ProcessArgs& args) override {
	}

    void setImageData(ImageData data)
    {
        image_data_ = std::move(data);
    }

private:
    ImageData image_data_;
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
        module_->setImageData(image_data_);
    }

    void draw(DrawArgs const& args) override
    {
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

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(17.357, 87.598)), module, PictureThis::R_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(17.089, 99.902)), module, PictureThis::G_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(17.351, 111.929)), module, PictureThis::B_OUTPUT));

		// mm2px(Vec(116.851, 116.851))
		addChild(createWidget<Widget>(mm2px(Vec(29.864, 5.502))));

        // Add a demo image
        auto* image = new PngWidget(module);
        image->box.pos = Vec{100.0f, 50.0f};
        image->box.size = Vec{box.size.x - 150.0f, box.size.y - 100.0f};
        image->setImage(asset::plugin(pluginInstance, "res/Test.png"));
        addChild(image);

        std::string str = std::string{"hi"};
    }
};

Model* modelPictureThis = createModel<PictureThis, PictureThisWidget>("PictureThis");
