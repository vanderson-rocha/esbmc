double fabs(double);
int abs(int);
int isnan(double);

int main()
{
  int my_i, iabs;
  double my_d, dabs;
  
  assert(abs(-1)==1);
  assert(abs(1)==1);
  assert(fabs(1.0)==1);
  assert(fabs(-1.0)==1);
  
  iabs=(my_i<0)?-my_i:my_i;
  assert(abs(my_i)==iabs);

  __ESBMC_assume(!isnan(my_d));
 
  dabs=(my_d<0)?-my_d:my_d;
  assert(fabs(my_d)==dabs);
}
