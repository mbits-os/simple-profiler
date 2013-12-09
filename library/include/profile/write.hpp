#ifndef __WRITE_HPP__
#define __WRITE_HPP__

#ifdef FEATURE_IO_WRITE

namespace profile { namespace io {

	void xml_write(const char* filename);
	void binary_write(const char* filename);

	enum EWriter
	{
		EWriter_XML,
		EWriter_BIN
	};

	struct writer
	{
		const char* m_filename;
		EWriter    m_typeId;
		writer(const char* filename, EWriter typeId = EWriter_BIN)
			: m_filename(filename)
			, m_typeId(typeId)
		{}

		~writer()
		{
			switch (m_typeId)
			{
			case EWriter_XML: xml_write(m_filename); break;
			case EWriter_BIN: binary_write(m_filename); break;
			}
		}
	};

}} // profile::io

#endif // FEATURE_IO_WRITE

#endif // __WRITE_HPP__
