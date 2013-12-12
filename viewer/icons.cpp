#include "icons.h"

namespace icons
{
	QIcon iconFile(const QString& off, const QString& on)
	{
		QIcon icon;
		icon.addFile(off);
		icon.addFile(on, QSize(), QIcon::Normal, QIcon::On);
		return icon;
	}

	QVariant icon(EIconType type)
	{
		if (type == EIconType_None)
			return QVariant();

		static const QIcon icons[] =
		{
			iconFile(":/res/folder_clsed.png", ":/res/folder_opnd.png"),
			QIcon(":/res/node.png"),
			QIcon(":/res/node_section.png"),
			QIcon(":/res/node_syscall.png"),
			QIcon(":/res/node_ref.png")
		};

		return icons[type];
	}
}
