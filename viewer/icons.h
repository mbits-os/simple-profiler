#ifndef ICONS_H
#define ICONS_H

#include <QIcon>
#include <QVariant>

namespace icons
{
	enum EIconType
	{
		EIconType_Folder,
		EIconType_Function,
		EIconType_Section,
		EIconType_Syscall,
		EIconType_Reference,
		EIconType_None
	};

	QVariant icon(EIconType type);
}

#endif // ICONS_H
