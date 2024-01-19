#include<stdio.h>
#define MUL(a, b) \
(a * b)
int main()
{
int x = MUL(5 + 4, 3 + 2);
printf("%d", x);
return 0;
}