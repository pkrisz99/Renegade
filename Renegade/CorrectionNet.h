#pragma once

#include "Utils.h"

constexpr int CorrInputSize = 6;
constexpr int CorrL1 = 8;
constexpr int CorrL2 = 4;

struct CorrectionNet {

	// Network representation
	std::array<std::array<float, CorrL1>, CorrInputSize> InputWeights;
	std::array<float, CorrL1> InputBiases;
	std::array<std::array<float, CorrL2>, CorrL1> L1Weights;
	std::array<float, CorrL2> L1Biases;
	std::array<float, CorrL2> L2Weights;
	float L2Bias;

	float Propagate(const std::array<float, CorrInputSize>& inputs) {
		
		// Activation function
		auto ReLU = [](const float value) {
			return std::max(value, 0.f);
		};

		// Propagate inputs -> L1
		std::array<float, CorrL1> l1 = InputBiases;
		for (int i = 0; i < CorrInputSize; i++) {
			for (int j = 0; j < CorrL1; j++) {
				l1[j] = std::fma(InputWeights[i][j], inputs[i], l1[j]);
			}
		}
		for (int i = 0; i < CorrL1; i++) l1[i] = ReLU(l1[i]);

		// Propagate L1 -> L2
		std::array<float, CorrL2> l2 = L1Biases;
		for (int i = 0; i < CorrL1; i++) {
			for (int j = 0; j < CorrL2; j++) {
				l2[j] = std::fma(L1Weights[i][j], l1[i], l2[j]);
			}
		}
		for (int i = 0; i < CorrL2; i++) l2[i] = ReLU(l2[i]);

		// Propagate L2 -> output
		float out = L2Bias;
		for (int i = 0; i < CorrL2; i++) {
			out = std::fma(L2Weights[i], l2[i], out);
		}

		return out;
	}

	int Calculate(const int rawEval, const int depth, const bool pvNode, const int pawnCorrhist, const int materialCorrhist, const int followUpCorrhist) {

		auto Sigmoid = [](float x) {
			x = std::clamp(x, -20.f, 20.f);
			return 1.f / (1.f + std::exp(-x));
			};

		auto InverseSigmoid = [](float y) {
			return std::log(y / (1 - y));
			};

		// Transform inputs
		const std::array<float, CorrInputSize> inputs = {
			Sigmoid(static_cast<float>(rawEval) / 256.f),
			std::log(std::clamp(static_cast<float>(depth), 1.f, 10.f)),
			static_cast<float>(pvNode),
			static_cast<float>(pawnCorrhist) / 16.f, 
			static_cast<float>(materialCorrhist) / 16.f, 
			static_cast<float>(followUpCorrhist) / 16.f,
		};

		const float rawOutput = Propagate(inputs);
		const int finalOutput = std::clamp(static_cast<int>(InverseSigmoid(rawOutput) * 16.f), -48, 48);

		return finalOutput;
	}

	CorrectionNet() {
		InputWeights[0] = { -0.59905523, -0.15045132, -0.14635958, 0.47466698, 0.39983335, -0.7594871, -0.28707367, 1.2632806 };
		InputWeights[1] = { -0.4749362, 0.31107536, -0.5539968, 0.104662165, 0.60320926, 0.51256293, 0.4736044, 0.07329487 };
		InputWeights[2] = { -0.30510613, 0.36070052, -0.6264694, 0.098211534, 0.07003313, -0.23311086, 0.40697864, 0.04595036 };
		InputWeights[3] = { 0.266389, 0.5763706, 0.23481381, 0.22691596, -0.42345327, -0.52030313, 0.51227105, -0.32674745 };
		InputWeights[4] = { 0.4132104, 0.5783795, 0.04717189, -0.4709542, -0.062232077, -0.14001411, 0.08264601, 0.5635196 };
		InputWeights[5] = { -0.08905935, -0.31850308, -0.61982405, 0.35488474, -0.01801604, -0.38890898, -0.4457461, 0.3861133 };
		InputBiases = { 0.0, 0.15163061, 0.1466272, 0.061278015, 0.03220323, 0.11754413, -0.19397898, 0.059022415 };


		L1Weights[0] = { -0.13756394, -0.4833586, 0.5601935, 0.1675064 };
		L1Weights[1] = { -0.53214514, 0.39904508, -0.55236495, 0.78928334 };
		L1Weights[2] = { 0.40546748, 0.67211914, -0.6851157, 0.5003499 };
		L1Weights[3] = { -0.4527548, 0.23441091, 0.59177727, -0.005945791 };
		L1Weights[4] = { -0.08092185, 0.101926595, -0.5160733, -0.62621284 };
		L1Weights[5] = { -0.22511509, 0.22949296, 0.23168445, 0.8526907 };
		L1Weights[6] = { -0.066939496, -0.52043056, -0.52164274, -0.37489825 };
		L1Weights[7] = { 0.17781523, 0.10366036, -0.3611425, -1.056363 };
		L1Biases = { -0.049685694, 0.08963072, 0.0010031965, 0.035724185 };


		L2Weights[0] = { 0.014229221 };
		L2Weights[1] = { 0.9384948 };
		L2Weights[2] = { 0.0068440023 };
		L2Weights[3] = { -1.4280494 };
		L2Bias = { 0.09176639 };


	}
};
