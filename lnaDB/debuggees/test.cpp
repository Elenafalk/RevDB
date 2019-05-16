#include <iostream>

using namespace std;

int doThings() {
  int a = 5;
  cout << a << endl;
  return a;
}

int doStuff() {
  int b = 4;
  cout << b << endl;
  return b;
}

int main() {
  int c = doStuff() + doThings() + doStuff();
  cout << c << endl;
}