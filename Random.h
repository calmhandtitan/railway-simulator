class Random
{
private:
	random_device rd;
	default_random_engine e1;
	uniform_int_distribution<unsigned int> uniform_dist;
public:
	explicit Random(unsigned int max) : uniform_dist(1, max)
	{
		e1.seed(rd());
	}
	unsigned int generate()
	{
		return uniform_dist(e1);
	}

};

