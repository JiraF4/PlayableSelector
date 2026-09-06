// Compatibility implementation for SDK versions where the shared Tuple2 helper
// is not supplied by the base GameLib script module.
class Tuple2<Class T1, Class T2>
{
	T1 param1;
	T2 param2;

	void Tuple2(T1 p1, T2 p2)
	{
		param1 = p1;
		param2 = p2;
	}
}
