#include "project.h"

char *table = "0123456789ABCDEF";
int BASE10 = 10;
int BASE8 = 8;
int BASE16 = 16;
/*Use putchar() to write a prints() to print a string*/
void prints(char *s)
{
	char *cur = s;
	while (cur != NULL && *cur != '\0')
	{
		putchar(*cur);
		cur++;
	}
	return;
}
/*Helper function for printu()*/
void rpu(uint32_t x)
{
	char c;
	if (x)
	{
		c= table[x% BASE10];
		rpu(x/BASE10);
		putchar(c);
	}
}
/*Given printu() from assignment
* prints an unsigned int (u32) x
*/
void printu(uint32_t x)
{
	if (x==0)
	{
		putchar('0');
	}
	else
	{
		rpu(x);
	}
	//putchar(' ');
}

/*Helper function for printd*/
void rpd(int x)
{
	char c;
	if (x){
		c = table[x%BASE10];
		rpd(x/BASE10);
		putchar(c);
	}
}
/*prints an integer (may be negative or nonnegative)*/
int printd(int x)
{
	if(x < 0)
	{
		putchar('-');
		x = x * -1;
	}
	if(x ==0){
		putchar('0');
	}else{
		rpd(x);
	}
	//putchar(' ');
	return 0;
}
/*Helper function for printo()*/
void rpo(uint32_t x)
{
	char c;
	if (x)
	{
		c= table[x% BASE8];
		rpo(x/BASE8);
		putchar(c);
	}

}
/*
* prints (u32) x in octal
*/
void printo(uint32_t x)
{
	if (x==0)
	{
		putchar('0');
	}
	else
	{
		rpo(x);
	}
	//putchar(' ');
}
/*Helper function for printx()*/
void rpx(uint32_t x)
{
	char c;
	if (x)
	{
		c= table[x% BASE16];
		rpx(x/BASE16);
		putchar(c);
	}
}
/*
* prints an (u32) x in Hex
*/
void printx(uint32_t x)
{
	putchar('0');
	putchar('x');
	if (x==0)
	{
		putchar('0');
	}
	else
	{
		rpx(x);
	}
	//putchar(' ');
}
/*Should work like printf() for %c, %s,%u, %d, %o, %x*/
void debugMode(char *fmt, ...)
{
  if(DEBUG_STATUS == FALSE)
    return;

	char *cp = fmt;	//point cp to the fmt string
	int *ip = &fmt + 1;	//point at the first item to be printed
	char pType = '\0';	//parameter variable type

	while(*cp != '\0')
	{
		if(*cp == '%')
		{
			//get next char
			cp = cp + 1;
			pType = *cp;
			//switch goes here
			switch(pType){
				case 'c':
					putchar(*ip);
					break;
				case 's':
					prints(*ip);
					break;
				case 'u':
					printu(*ip);
					break;
				case 'd':
					printd(*ip);
					break;
				case 'o':
					printo(*ip);
					break;
				case 'x':
					printx(*ip);
					break;
			}
			//move to next parameter
			ip = ip + 1;
		}else{
			putchar(*cp);
		}
		//move down fmt string
		cp = cp + 1;
 	}
}
