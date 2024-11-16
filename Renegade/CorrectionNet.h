#pragma once

#include "Utils.h"

constexpr int CorrInputSize = 4;
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

	int Calculate(const int rawEval, const int pawnCorrhist, const int materialCorrhist, const int followUpCorrhist) {

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
			static_cast<float>(pawnCorrhist) / 16.f, 
			static_cast<float>(materialCorrhist) / 16.f, 
			static_cast<float>(followUpCorrhist) / 16.f,
		};

		const float rawOutput = Propagate(inputs);
		const int finalOutput = std::clamp(static_cast<int>(InverseSigmoid(rawOutput) * 16.f), -48, 48);

		return finalOutput;
	}

	CorrectionNet() {
		InputWeights[0] = { -0.38156036, 0.82084304, -0.24506092, -0.64146096, -0.045917135, -0.32689542, -0.8052867, 0.19849709 };
		InputWeights[1] = { -0.34859765, -0.15793869, -0.4760641, 0.25337216, 0.29980633, -0.0008703869, -0.048183676, 0.0677446 };
		InputWeights[2] = { -0.8661157, -0.5069679, 0.23926343, 0.6322531, 0.695964, -0.010046597, 0.4144339, 0.047759444 };
		InputWeights[3] = { 0.15134579, 0.081758544, 0.13566856, -0.033043955, -0.1874574, -0.68473047, -0.029916313, 0.6477948 };
		InputBiases = { 0.2721377, 0.14166167, 0.34285, -0.39254576, 0.04561554, 0.3296331, 0.38830087, 0.09303348 };


		L1Weights[0] = { 0.009297094, -0.22623359, -0.7191129, 0.44785884 };
		L1Weights[1] = { 0.45435768, 0.33557272, 0.19715796, -0.51828665 };
		L1Weights[2] = { -0.07050063, -0.3442808, 0.24741834, -0.33697006 };
		L1Weights[3] = { -0.30151352, 0.6256786, 0.3660513, 0.14453727 };
		L1Weights[4] = { 0.66170585, 0.876289, -0.54871905, -0.18074016 };
		L1Weights[5] = { -0.015532728, -0.5710424, -0.76647115, 0.09749917 };
		L1Weights[6] = { -0.62192374, -0.07479451, 0.2851431, 0.47660026 };
		L1Weights[7] = { 0.13123457, 0.7050667, -0.60698134, 0.066907406 };
		L1Biases = { -0.37663493, 0.40266955, -0.17021503, 0.055726163 };


		L2Weights[0] = { -0.4884531 };
		L2Weights[1] = { 0.31653523 };
		L2Weights[2] = { -0.41412982 };
		L2Weights[3] = { -0.65676266 };
		L2Bias = { 0.27499062 };

	}
};
