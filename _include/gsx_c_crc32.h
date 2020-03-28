#ifndef __GSX_C_CRC32_H__
#define __GSX_C_CRC32_H__

#include <stddef.h>
class CCRC32
{
private:
	CCRC32();

public:
	~CCRC32();

private:
	static CCRC32 *m_instance;

public:
	static CCRC32 *GetInstance()
	{
		if (m_instance == NULL)
		{
			if (m_instance == NULL)
			{
				m_instance = new CCRC32();
				static CGarhuishou cl;
			}
		}
		return m_instance;
	}
	class CGarhuishou
	{
	public:
		~CGarhuishou()
		{
			if (CCRC32::m_instance)
			{
				delete CCRC32::m_instance;
				CCRC32::m_instance = NULL;
			}
		}
	};

public:
	void Init_CRC32_Table();
	unsigned int Reflect(unsigned int ref, char ch);
	int Get_CRC(unsigned char *buffer, unsigned int dwSize);

public:
	unsigned int crc32_table[256];
};

#endif
