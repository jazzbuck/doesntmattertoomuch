#include "plugin.hpp"


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
	}
};


Model* modelPictureThis = createModel<PictureThis, PictureThisWidget>("PictureThis");