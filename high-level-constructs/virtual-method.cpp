struct Foo {
  int a;

  virtual int getLengthTimesTwo() { return a * 2; }
  void setLength(int aa) { a = aa; }
};

int main(int argc, char **argv) {
  Foo f;
  f.setLength(4);
  return f.getLengthTimesTwo();
}
