#ifndef TEST_RUNTIME_HPP_
#define TEST_RUNTIME_HPP_

void param(int parameter_id, int value);
void on(int input_id);
void off(int input_id);
void delay(double seconds);
void jump(double seconds);
double time();

#endif
