
#include "sqrt.h"

double
sqrt(const double n)
{
	double	lower;
	double	upper;
	double	sqrt;

	if (n <= 0)
		return (0.0);

	if (n < 1)
	{
		lower = n;
		upper = 1;
	}
	else
	{
		lower = 1;
		upper = n;
	}

	while ((upper - lower) > SQRT_ACCURACY)
	{
		sqrt = (upper + lower) / 2;

		if (sqrt * sqrt > n)
			upper = sqrt;
		else
			lower = sqrt;
	}
	return ((upper + lower) / 2);
}
