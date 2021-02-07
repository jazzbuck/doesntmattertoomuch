#include "plugin.hpp"
#include <nanovg.h>
#include <stb_image.h>
#include <widget/TransparentWidget.hpp>

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
};

struct PngWidget : TransparentWidget
{
    explicit PngWidget()
    {}

    void setImagePath(std::string new_path)
    {
        image_path_ = std::move(new_path);
        new_image_path_ = true;

        unsigned char* ptr = stbi_load(image_path_.c_str(), &width_, &height_, &comp_, 4);
        image_data_.reset(ptr);
    }

    void draw(DrawArgs const& args) override
    {
        int const handle = nvgCreateImageRGBA(args.vg, width_, height_, 0, image_data_.get());

        nvgBeginPath(args.vg);

        auto const scale_factor_x = box.size.x / width_;
        auto const scale_factor_y = box.size.y / height_;
        auto const scale_factor = std::max(scale_factor_x, scale_factor_y);
        nvgScale(args.vg, scale_factor, scale_factor);

        NVGpaint const paint = nvgImagePattern(args.vg, x_, y_, width_, height_, 0.0f, handle, 1.0f);
        nvgRect(args.vg, x_, y_, width_, height_);
        nvgFillPaint(args.vg, paint);
        nvgFill(args.vg);

        nvgClosePath(args.vg);
    }

private:
    struct Deleter
    {
        void operator()(unsigned char* ptr)
        {
            stbi_image_free(ptr);
        }
    };

    using ImagePtr = std::unique_ptr<unsigned char, Deleter>;

    std::string image_path_{};
    bool new_image_path_{};

    ImagePtr image_data_{};
    int height_{};
    int width_{};
    int comp_{};
    int x_{};
    int y_{};
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
        auto* image = new PngWidget();
        image->box.pos = Vec{100.0f, 50.0f};
        image->box.size = Vec{box.size.x - 150.0f, box.size.y - 100.0f};
        image->setImagePath(asset::plugin(pluginInstance, "res/Test.png"));
        addChild(image);
    }
};

Model* modelPictureThis = createModel<PictureThis, PictureThisWidget>("PictureThis");
