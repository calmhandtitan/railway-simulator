template< typename Stream >
class AtomicLogger 
{
	mutable mutex m_mutex;
	Stream& m_stream;
public:

	explicit AtomicLogger(Stream& stream) :
		m_mutex(),
		m_stream(stream)
	{
	}

	void log(const string& str) {
		unique_lock<mutex> lock(m_mutex);
		m_stream << str << endl;
	}
};

typedef AtomicLogger< ostream > AtomicLoggerOstream;
typedef shared_ptr< AtomicLoggerOstream > AtomicLoggerOstreamPtr;

