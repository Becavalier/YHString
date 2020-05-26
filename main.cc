#include <iostream>
#include "include/YHString.h"

int main(int argc, char **argv) {
  std::cout << std::boolalpha;
  
  // SSO: basic constructing, copy-constructing, and assignment.
  YHString strA("Hello, world!"), strB(strA), strC;
  strC = strA;
  std::cout << strA << std::endl;
  std::cout << strB << std::endl;
  std::cout << strC << std::endl;
  std::cout << (strA == strB) << std::endl;  // true;

  // COW: sharing dynamic memory; 
  YHString strD(R"(
    YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY
    YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY 
    YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY
    YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY
    YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY
  )");
  YHString strE(strD);
  std::cout << isSharing(strD, strE) << std::endl;  // true.
  
  // COW: sharing assignment.
  strC = strD;
  std::cout << strC << std::endl;
  std::cout << isSharing(strC, strE) << std::endl;  // true.
  
  // COW: non-sharing assignment.
  YHString strF(R"(
    HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO 
    HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO 
    HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO 
    HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO 
    HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO
  )");

  strC = strF;
  std::cout << strC << std::endl;
  std::cout << isSharing(strC, strE) << std::endl;  // false.
  
  // COW: non-sharing operator[].
  for (auto i = 0; i < 10; ++i) { strD[i] = 'c'; }
  
  std::cout << strD << std::endl;
  std::cout << isSharing(strD, strE) << std::endl;  // false.

  // eager-copy: constructing.
  YHString strG = "Hello, there are still many things to do.";
  std::cout << strG << std::endl;

  // eager-copy: out-of-bound exception thrown;
  // strG[225] = 'c';

  std::cout << strG.length() << std::endl;
  std::cout << (strG == strD) << std::endl;  // false;
  std::cout << (strG == strG) << std::endl;  // true;
	
  YHString strH(YHString("Hello, it's a godd day! Do you want some coffee?"));
  std::cout << strH << std::endl;
  return 0;
}

