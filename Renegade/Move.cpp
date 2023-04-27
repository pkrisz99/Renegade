#include "Move.h"

Move::Move() {
	this->from = 0;
	this->to = 0;
	flag = 0;
}

Move::Move(uint8_t from, uint8_t to) {
	this->from = from;
	this->to = to;
	flag = 0;
}

Move::Move(uint8_t from, uint8_t to, uint8_t flag) {
	this->from = from;
	this->to = to;
	this->flag = flag;
}
void Move::SetFlag(uint8_t flag) {
	this->flag = flag;
}

const std::string Move::ToString() const {
	if ((from == 0) && (to == 0)) return "0000";

	int file1 = from % 8;
	int rank1 = from / 8;
	int file2 = to % 8;
	int rank2 = to / 8;

	char f1 = 'a' + file1;
	char r1 = '1' + rank1;
	char f2 = 'a' + file2;
	char r2 = '1' + rank2;

	char extra = '?';
	if (flag == MoveFlag::PromotionToKnight) extra = 'n';
	if (flag == MoveFlag::PromotionToBishop) extra = 'b';
	if (flag == MoveFlag::PromotionToRook) extra = 'r';
	if (flag == MoveFlag::PromotionToQueen) extra = 'q';

	if (extra == '?') return { f1, r1, f2, r2 };
	else return { f1, r1, f2, r2, extra };

}

const bool Move::IsNotNull() {
	return (from != 0) || (to != 0);
}

const bool Move::IsEmpty() {
	return (from == 0) && (to == 0) && (flag == 0);
}

const bool Move::IsUnderpromotion() {
	return (flag == MoveFlag::PromotionToRook) || (flag == MoveFlag::PromotionToKnight) || (flag == MoveFlag::PromotionToBishop);
}

const bool Move::IsPromotion() const {
	return (flag == MoveFlag::PromotionToQueen) || (flag == MoveFlag::PromotionToRook) 
		|| (flag == MoveFlag::PromotionToKnight) || (flag == MoveFlag::PromotionToBishop);
}

const uint8_t Move::GetPromotionPieceType() const {
	switch (flag) {
	case MoveFlag::PromotionToQueen: return PieceType::Queen;
	case MoveFlag::PromotionToRook: return PieceType::Rook;
	case MoveFlag::PromotionToKnight: return PieceType::Knight;
	case MoveFlag::PromotionToBishop: return PieceType::Bishop;
	default: return PieceType::None;
	}
}