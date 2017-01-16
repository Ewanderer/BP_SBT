# author: Jordan Eichner
#pragma once

template<class T,unsigned size>
class Ringbuffer
{
	
	T buffer[size];
	unsigned head, tail;
public:
	ringbuffer(){
		head = 0;
		tail = 0;
		/*for (int i = 0; i < size;i++)
			buffer[i]=T();*/
	}
	
	//returns true if sucessful
	bool push(T element){
		if ((head+1) % (size+1) != tail){
			buffer[head] = element;
			head = (head + 1) % (size+1);
			return true;
		}
		return false;
	}

	//return next element, check if available first!
	T pop(){
		T result=T();
		if (head != tail){
			result = buffer[tail];
			tail = (tail + 1) % (size+1);
			
		}
		return result;
	}
	
	bool able(){
		return (head != tail);
	}
	
	int rem_slots(){
		return 0;		
	}

};
