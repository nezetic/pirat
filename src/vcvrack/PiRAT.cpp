/**
 * VCV Rack implementation of PiRAT distortion, featuring:
 *
 * - mono signal path
 * - virtual knobs with dedicated CV controls
 * - hard clip toggle
 * - "lovely" panel
 *
 */
#include "plugin.hpp"

#include "PiRATDist.h"
#include "NoiseGate.h"

using namespace pirat;


struct PiRAT : Module {
    enum ParamId {
        GAIN_PARAM,
        FILTER_PARAM,
        LEVEL_PARAM,
        MIX_PARAM,
        HARD_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        GAIN_CV_INPUT,
        FILTER_CV_INPUT,
        LEVEL_CV_INPUT,
        MIX_CV_INPUT,
        IN_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        OUT_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        GAIN_LIGHT,
        LIGHTS_LEN
    };

    PiRAT() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(GAIN_PARAM, 0.f, 1.f, 0.f, "Voltage gain");
        configParam(FILTER_PARAM, 0.f, 1.f, 0.f, "Tone");
        configParam(LEVEL_PARAM, 0.f, 1.f, 0.20f, "Input level");
        configParam(MIX_PARAM, 0.f, 1.f, 1.f, "Dry/Wet (Si/Led) mix");
        configSwitch(HARD_PARAM, 0, 1, 1, "Hard (led clip)", {"Off", "On"});
        configInput(GAIN_CV_INPUT, "Gain CV");
        configInput(FILTER_CV_INPUT, "Tone CV");
        configInput(LEVEL_CV_INPUT, "Level CV");
        configInput(MIX_CV_INPUT, "Mix CV");
        configInput(IN_INPUT, "Audio IN");
        configOutput(OUT_OUTPUT, "Audio OUT");

        configBypass(IN_INPUT, OUT_OUTPUT);

        float fs = APP->engine->getSampleRate();
        dist.Init(fs);
        ng.Init(fs);

        ng.SetParam(NoiseGate::P_DETECTOR_GAIN, 0.5);  // * 1
        ng.SetParam(NoiseGate::P_REDUCTION, 0.4);      // -40db
        ng.SetParam(NoiseGate::P_THRESHOLD, 0.4);      // -45db
        ng.SetParam(NoiseGate::P_SLOPE, 0.3);          // 3
        ng.SetParam(NoiseGate::P_RELEASE, 0.5, true);  // 100ms
    }

    void onSampleRateChange() override {
        float fs = APP->engine->getSampleRate();
        dist.SetSampleRate(fs);
        ng.SetSampleRate(fs);
    }

    void process(const ProcessArgs& args) override {
        if (--cycle_count < 0) {
            cycle_count = 3;
        }
        const float level_cv = clamp(inputs[LEVEL_CV_INPUT].getVoltage(), -5.f, 5.f) / 5.f;
        const float gain_cv = clamp(inputs[GAIN_CV_INPUT].getVoltage(), -5.f, 5.f) / 5.f;
        const float filter_cv = clamp(inputs[FILTER_CV_INPUT].getVoltage(), -5.f, 5.f) / 5.f;
        const float mix_cv = clamp(inputs[MIX_CV_INPUT].getVoltage(), -5.f, 5.f) / 5.f;

        const float hard = params[HARD_PARAM].getValue();

        dist.SetParam(PiRATDist::P_GAIN, params[GAIN_PARAM].getValue() + gain_cv);
        dist.SetParam(PiRATDist::P_FILTER, params[FILTER_PARAM].getValue() + filter_cv);
        dist.SetParam(PiRATDist::P_LEVEL, params[LEVEL_PARAM].getValue() + level_cv);
        // in hard mode, mix Silicon / Led clippers
        if (hard >= 0.5f) {
            dist.SetParam(PiRATDist::P_DRYWET, 1.f);
            dist.SetParam(PiRATDist::P_SILED, params[MIX_PARAM].getValue() + mix_cv);
        } else {
            dist.SetParam(PiRATDist::P_DRYWET, params[MIX_PARAM].getValue() + mix_cv);
        }
        dist.SetParam(PiRATDist::P_HARD, hard);

        // update distortion heavy params every 4 blocks
        if (cycle_count == 0) {
            dist.Update();
        }

        const float input = inputs[IN_INPUT].getVoltage();
        const float out = dist.Process(input);
        const float gated = ng.Process(out, input);
        outputs[OUT_OUTPUT].setVoltage(gated);

        lights[GAIN_LIGHT].setBrightness(dist.GetSaturation());
    }

    int cycle_count = 1;
    pirat::PiRATDist dist;
    pirat::NoiseGate ng;
};


struct PiRATWidget : ModuleWidget {
    PiRATWidget(PiRAT* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/PiRAT.svg")));

        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(12.0, 22.179)), module, PiRAT::GAIN_PARAM));
        addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(39.052, 22.179)), module, PiRAT::FILTER_PARAM));
        addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(12.0, 68.521)), module, PiRAT::LEVEL_PARAM));
        addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(39.052, 68.521)), module, PiRAT::MIX_PARAM));
        addParam(createParamCentered<NKK>(mm2px(Vec(25.424, 78.839)), module, PiRAT::HARD_PARAM));

        addInput(createInputCentered<PJ3410Port>(mm2px(Vec(8.655, 45.267)), module, PiRAT::GAIN_CV_INPUT));
        addInput(createInputCentered<PJ3410Port>(mm2px(Vec(41.63, 45.267)), module, PiRAT::FILTER_CV_INPUT));
        addInput(createInputCentered<PJ3410Port>(mm2px(Vec(14.049, 90.588)), module, PiRAT::LEVEL_CV_INPUT));
        addInput(createInputCentered<PJ3410Port>(mm2px(Vec(37.241, 90.588)), module, PiRAT::MIX_CV_INPUT));
        addInput(createInputCentered<PJ3410Port>(mm2px(Vec(13.382, 110.072)), module, PiRAT::IN_INPUT));

        addOutput(createOutputCentered<PJ3410Port>(mm2px(Vec(38.317, 110.072)), module, PiRAT::OUT_OUTPUT));

        addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(28.789, 30.026)), module, PiRAT::GAIN_LIGHT));
    }
};


Model* modelPiRAT = createModel<PiRAT, PiRATWidget>("PiRAT");
