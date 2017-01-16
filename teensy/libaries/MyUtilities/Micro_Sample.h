#pragma once

#include <cstdint>

template<unsigned size>
class Micro_Sample{
public:
	//returns true if full
	bool add_raw(uint16_t point){
		raw[c++]=point;
		if (point < min)
			min = point;
		if (max < point)
			max = point;
		sum += point;
		return c%size == 0;
	}
	void process(){
		uint16_t med = (uint16_t)sum / size;
		for (int i = 0; i < c; i++){
			processed[i] = raw[i] - med;
		}
	}

	Micro_Sample(unsigned long start){
		c = 0;
		min = 0;
		min--;
		max = 0;
		sum = 0;
		for (int i = 0; i < size; i++){
			raw[i] = 0;
			processed[i] = 0;
		}
		_start=start;
		_end=0;
		_end--;
	}
	
	unsigned long duration(){
		return _end-_start;
	}
	unsigned long _end;
	unsigned c;
	uint16_t raw[size];
	int32_t processed[size];
	uint16_t min, max, sum;
	unsigned long _start;

};