#include "Move.h"

Move::Move(int from, int to) {
	this->from = from;
	this->to = to;
	flag = 0;
}

Move::Move(int from, int to, int flag) {
	this->from = from;
	this->to = to;
	this->flag = flag;
}
void Move::SetFlag(int flag) {
	this->flag = flag;
}

std::string Move::ToString() {
	int file1 = from % 8;
	int rank1 = from / 8;
	int file2 = to % 8;
	int rank2 = to / 8;

	char f1 = 'a' + file1;
	char r1 = '1' + rank1;
	char f2 = 'a' + file2;
	char r2 = '1' + rank2;

	char extra = '?';
	if (flag == PromotionToKnight) extra = 'n';
	if (flag == PromotionToBishop) extra = 'b';
	if (flag == PromotionToRook) extra = 'r';
	if (flag == PromotionToQueen) extra = 'q';

	if (extra == '?') return { f1, r1, f2, r2 };
	else return { f1, r1, f2, r2, extra };

}