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
			std::log(std::clamp(static_cast<float>(depth), 1.f, 10.f)) / 2.f,
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
		InputWeights[0] = { 0.26169813, -0.07030369, -0.78403056, 0.1994907, -0.29809695, -0.9410766, 0.7668877, 0.0027677354 };
		InputWeights[1] = { -0.07734055, -0.15387815, 0.4368886, 0.25930783, -0.044458672, -0.23488267, -0.31409326, 0.079599865 };
		InputWeights[2] = { 0.24217263, -0.17910513, -0.097911485, -0.46063957, 0.055652946, -0.4714056, -0.57417285, 0.018039558 };
		InputWeights[3] = { 0.114580035, 0.025832195, -0.33667937, 0.26787063, -0.22728576, 0.0312735, 0.12265711, 0.016088374 };
		InputWeights[4] = { 0.5165632, 0.004477377, 0.05913926, 0.53952163, -0.7871589, -0.082860135, -0.029020797, -0.6424593 };
		InputWeights[5] = { 0.042557128, -0.43240613, 0.4613541, 0.02664367, -0.105318785, -0.10005389, -0.031391803, 0.029343123 };
		InputBiases = { -0.1669066, 0.18829903, -0.037244745, -0.14538124, 0.17664947, 0.33125374, -0.4089943, 0.6823712 };


		L1Weights[0] = { 0.3239605, 0.39430466, -0.23972249, -0.5138085 };
		L1Weights[1] = { 0.14170863, -0.4360273, -0.03444394, 0.26332626 };
		L1Weights[2] = { 0.086433984, -0.052123297, 0.24588639, 0.3589501 };
		L1Weights[3] = { 0.19792652, 0.21439622, 0.36544287, -0.780483 };
		L1Weights[4] = { -0.6007797, -0.26894423, -0.3354423, -0.57508403 };
		L1Weights[5] = { -0.8790374, -0.35263973, -0.7142101, -0.03293391 };
		L1Weights[6] = { 0.27655062, -0.35578883, -0.85358447, -0.63687736 };
		L1Weights[7] = { 0.57381797, 0.28468797, 0.2571776, -0.6065795 };
		L1Biases = { -0.06834446, 0.008455724, 0.3809172, -0.30674285 };


		L2Weights[0] = { 0.26751402 };
		L2Weights[1] = { 0.7444369 };
		L2Weights[2] = { 0.45713967 };
		L2Weights[3] = { 0.701541 };
		L2Bias = { 0.028042836 };

	}
};
