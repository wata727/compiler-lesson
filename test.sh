#!/bin/bash
cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int ret5() { return 5; }
int add(int x, int y) { return x+y; }
int sub(int x, int y) { return x-y; }

int add6(int a, int b, int c, int d, int e, int f) {
  return a+b+c+d+e+f;
}
EOF

try() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  gcc -o tmp tmp.s tmp2.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$expected expected, but got $actual"
    exit 1
  fi
}

try 0 "int main() { return 0; }"
try 42 "int main() { return 42; }"
try 21 "int main() { return 5+20-4; }"
try 41 "int main() { return 12 + 34 - 5 ; }"
try 47 "int main() { return 5+6*7; }"
try 15 "int main() { return 5*(9-6); }"
try 4 "int main() { return (3+5)/2; }"
try 10 "int main() { return -10+20; }"
try 0 "int main() { return 0==1; }"
try 1 "int main() { return 42==42; }"
try 1 "int main() { return 0!=1; }"
try 0 "int main() { return 42!=42; }"
try 1 "int main() { return 0<1; }"
try 0 "int main() { return 1<1; }"
try 0 "int main() { return 2<1; }"
try 1 "int main() { return 0<=1; }"
try 1 "int main() { return 1<=1; }"
try 0 "int main() { return 2<=1; }"
try 1 "int main() { return 1>0; }"
try 0 "int main() { return 1>1; }"
try 0 "int main() { return 1>2; }"
try 1 "int main() { return 1>=0; }"
try 1 "int main() { return 1>=1; }"
try 0 "int main() { return 1>=2; }"
try 3 "int main() { 1;2; return 3; }"
try 3 "int main() { int a; a=3; return a; }"
try 8 "int main() { int a; a=3; int z; z=5; return a+z; }"
try 3 "int main() { int foo; foo=3; return foo; }"
try 8 "int main() { int foo123; foo123=3; int bar; bar=5; return foo123+bar; }"
try 3 "int main() { if (0) return 2; return 3; }"
try 3 "int main() { if (1-1) return 2; return 3; }"
try 3 "int main() { if (0) return 2; else return 3; }"
try 2 "int main() { if (1) return 2; return 3; }"
try 2 "int main() { if (2-1) return 2; return 3; }"
try 2 "int main() { if (1) return 2; else return 3; }"
try 10 "int main() { int i; i=0; while(i<10) i=i+1; return i; }"
try 0 "int main() { int i; i=0; while(i>10) i=i+1; return i; }"
try 55 "int main() { int i; i=0; int j; j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }"
try 0 "int main() { int i; i=0; int j; j=0; for (i=0; i>=10; i=i+1) j=i+j; return j; }"
try 3 "int main() { for (;;) return 3; return 5; }"
try 3 "int main() { {1; {2;} return 3;} }"
try 55 "int main() { int i; i=0; int j; j=0; while(i<=10) {j=i+j; i=i+1;} return j; }"
try 3 "int main() { return ret3(); }"
try 5 "int main() { return ret5(); }"
try 8 "int main() { return add(3, 5); }"
try 2 "int main() { return sub(5, 3); }"
try 21 "int main() { return add6(1,2,3,4,5,6); }"
try 32 "int main() { return ret32(); } int ret32() { return 32; }"
try 7 "int main() { return add2(3,4); } int add2(int x, int y) { return x+y; }"
try 1 "int main() { return sub2(4,3); } int sub2(int x, int y) { return x-y; }"
try 55 "int main() { return fib(9); } int fib(int x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }"
try 3 "int main() { int x; x=3; return *&x; }"
try 3 "int main() { int x; x=3; int *y; y=&x; int **z; z=&y; return **z; }"
try 5 "int main() { int x; x=3; int y; y=5; return *(&x-1); }"
try 3 "int main() { int x; x=3; int y; y=5; return *(&y+1); }"
try 5 "int main() { int x; x=3; int *y; y=&x; *y=5; return x; }"
try 7 "int main() { int x; x=3; int y; y=5; *(&x-1)=7; return y; }"
try 7 "int main() { int x; x=3; int y; y=5; *(&y+1)=7; return x; }"
try 8 "int main() { int x; x=3; int y; y=5; return foo(&x, y); } int foo(int *x, int y) { return *x + y; }"

echo OK
