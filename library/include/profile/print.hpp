#ifndef __PRINT_HPP__
#define __PRINT_HPP__

namespace profile { namespace io {

	void xml_print(const char* filename);
	void binary_print(const char* filename);

	enum EPrinter
	{
		EPrinter_XML,
		EPrinter_BIN
	};

	struct printer
	{
		const char* m_filename;
		EPrinter    m_typeId;
		printer(const char* filename, EPrinter typeId = EPrinter_BIN)
			: m_filename(filename)
			, m_typeId(typeId)
		{}

		~printer()
		{
			switch (m_typeId)
			{
			case EPrinter_XML: xml_print(m_filename); break;
			case EPrinter_BIN: binary_print(m_filename); break;
			}
		}
	};

}} // profile::io

#endif // __PRINT_HPP__
