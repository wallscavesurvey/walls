#include "QvNode.h"
#include "QvLists.h"

QvNodeList::QvNodeList() : QvPList()
{
}

void
QvNodeList::append(QvNode *node)
{
	QvPList::append(node);

	node->ref();
}

void
QvNodeList::remove(int which)
{
	if ((*this)[which] != NULL)
		(*this)[which]->unref();

	QvPList::remove(which);
}

void
QvNodeList::truncate(int start)
{
	for (int i = start; i < getLength(); i++)
		if ((*this)[i] != NULL)
			(*this)[i]->unref();

	QvPList::truncate(start);
}
