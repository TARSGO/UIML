
#include <cstdint>
#include <cstdio>
#include <cstring>

template <size_t _strlength>
class StaticString {
public:
	StaticString() {
		m_store[0] = '\0';
	}
	
	StaticString(const char * const initString) {
		std::strncpy(m_store, initString, _strlength);
		m_store[_strlength - 1] = '\0';
	}
	
	char * begin() { return m_store; }
	char * end() { return m_store + size(); }
	
	operator const char *() const { return m_store; } 
	
	size_t size() const { return std::strlen(m_store); }
	void clear() { m_store[0] = '\0'; }
	
	// Appending string
	friend StaticString<_strlength>& operator<<(StaticString<_strlength>& self, const char * extra) {
		std::strncat(self.m_store, extra, _strlength - 1);
		self.m_store[_strlength] = '\0';
		return self;
	}
	
	// Appending character
	friend StaticString<_strlength>& operator<<(StaticString<_strlength>& self, char extra) {
		auto sz = self.size();
		if (sz == _strlength - 1) {
			return self;
		} else {
			self.m_store[sz] = extra;
			self.m_store[sz + 1] = '\0';
		}
		return self;
	}
	
	// Appending unsigned integer
	friend StaticString<_strlength>& operator<<(StaticString<_strlength>& self, uint32_t extra) {
		std::snprintf(self.end(), _strlength - self.size(), "%d", extra);
		self.m_store[_strlength] = '\0';
		return self;
	}
	
	// Appending signed integer
	friend StaticString<_strlength>& operator<<(StaticString<_strlength>& self, int32_t extra) {
		std::snprintf(self.end(), _strlength - self.size(), "%d", extra);
		self.m_store[_strlength] = '\0';
		return self;
	}
	
	// Guard
	static_assert(_strlength != 0, "StaticString must not be zero length");
	
private:
	char m_store[_strlength];
};


