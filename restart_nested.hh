#ifndef RESTART_NESTED_HH
#define RESTART_NESTED_HH

template<class OuterRestart, class InnerRestart>
class restart_nested {
public:
	OuterRestart outer;
	InnerRestart inner;

	restart_nested()
	{
	}

	bool operator()()
	{
		if (inner()) {
			if (outer())
				inner = InnerRestart();

			return true;
		}

		return false;
	}
};

#endif
