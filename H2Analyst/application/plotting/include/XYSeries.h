#pragma once

#include "Plottable.h"
#include "DataStructures.h"
#include "DataOperations.h"
#include "Dialogs.h"

#include <algorithm>

class XYSeries : public Plottable
{
	Q_OBJECT

	QCPCurve* m_Curve;

public:

	XYSeries(QCustomPlot* plot, const std::vector<const H2A::Dataset*> datasets);
	~XYSeries();

	void setAxisLabels();
	void setColor(QColor color);

	const QPointF dataAt(double time) const override;

};

