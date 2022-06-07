#include "PlotWidget.h"


PlotWidget::PlotWidget(QWidget* parent) : QCustomPlot(parent),
m_DataPanel(nullptr),
m_Datasets(),
m_TimeRange(0.0, 10.0),
m_DataRange(0.0, 5.0)
{
	this->xAxis->setRange(m_TimeRange);
	this->xAxis->setLabel("Time [sec]");
	this->yAxis->setRange(m_DataRange);

	// Right-click menu
	this->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));

	// User interactions
	this->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables | QCP::iMultiSelect);
	this->setAcceptDrops(true);

	// Connect signals
	connect(this->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(emitRangeChanged()));
	connect(this->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(enforceAxisLimits()));
	connect(this->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(enforceAxisLimits()));
}

bool PlotWidget::isEmpty()
{
	return m_Datasets.size() == 0;
}

void PlotWidget::setDataPanel(const DataPanel* datapanel) { m_DataPanel = datapanel;  }


void PlotWidget::setDatasets(const H2A::Dataset* dataset, bool replot)
{
	m_Datasets.clear();
	this->addDatasets(dataset, replot);
}


void PlotWidget::setDatasets(std::vector<const H2A::Dataset*> datasets, bool replot)
{
	m_Datasets.clear();
	this->addDatasets(datasets, replot);
}


void PlotWidget::addDatasets(const H2A::Dataset* dataset, bool replot)
{
	// Make sure dataset is populated
	m_DataPanel->requestDatasetPopulation(dataset, true);
	// Only add the dataset
	if (std::find(m_Datasets.begin(), m_Datasets.end(), dataset) == m_Datasets.end())
		m_Datasets.push_back(dataset);
	if (replot)
		this->plot();
}

void PlotWidget::addDatasets(std::vector<const H2A::Dataset*> datasets, bool replot)
{
	for (auto const& dataset : datasets)
		this->addDatasets(dataset, false);
	if (replot)
		this->plot();
}


void PlotWidget::plotSelected()
{
	this->setDatasets(m_DataPanel->getSelectedDatasets(), true);
}


void PlotWidget::plot()
{
	this->clearPlottables();

	// Add datasets
	double xmin = 0.0, xmax = 0.0, ymin = 0.0, ymax = 0.0;
	size_t plot_counter = 0;
	for (const auto& dataset : m_Datasets)
	{
		// Create QVectors from dataset data
		QVector<double> x(dataset->timeVec.begin(), dataset->timeVec.end());
		QVector<double> y(dataset->dataVec.begin(), dataset->dataVec.end());

		// Expand bounds with the new data
		xmin = std::min({ xmin, *std::min_element(x.begin(), x.end()) });
		xmax = std::max({ xmax, *std::max_element(x.begin(), x.end()) });
		ymin = std::min({ ymin, *std::min_element(y.begin(), y.end()) });
		ymax = std::max({ ymax, *std::max_element(y.begin(), y.end()) });

		// Add graph and data
		this->addGraph();
		this->graph()->setLineStyle(QCPGraph::lsStepLeft);
		this->graph()->setData(x, y);

		// Set visual properties
		this->graph()->setPen(QPen(k_PlotColors[plot_counter % k_PlotColors.size()]));
		QPen pen = this->graph()->pen();
		// Change pen to change appearance of selected plot
		this->graph()->selectionDecorator()->setPen(pen);

		++ plot_counter;
	}

	this->setAxisLabels();

	// Store and set axis ranges
	m_TimeRange = QCPRange(xmin, xmax);
	this->xAxis->setRange(m_TimeRange);
	m_DataRange = QCPRange(ymin, ymax);
	if (m_DataRange.lower > 0.0) m_DataRange.lower = 0.0;
	this->yAxis->setRange(m_DataRange);

	this->replot();
}

void PlotWidget::setAxisLabels()
{
	// Create unique list of quantities of datasets
	std::vector<std::string> units;
	for (const auto& dataset : m_Datasets)
	{
		if (std::find(units.begin(), units.end(), dataset->unit) == units.end())
		{
			units.push_back(dataset->unit);
		}
	}

	std::stringstream label;
	if (units.size() < 2)
	{
		// Since only a unique unit was found, we can assume there is also only one unique quantity
		label << m_Datasets.front()->quantity << " [" << units.front() << "]";
	}
	else
	{
		label << "Mixed quantities [";
		for (size_t i = 0; i < units.size(); ++ i)
		{
			label << units[i];
			if (i < units.size() - 1) label << ", ";
		}
		label << "]";
	}
	this->yAxis->setLabel(QString(label.str().c_str()));
	this->xAxis->setLabel("Time [sec]");
}


void PlotWidget::showContextMenu(const QPoint& pos)
{
	QMenu contextMenu(tr("Context menu"), this);

	QAction acPlot("Plot", this);
	connect(&acPlot, SIGNAL(triggered()), this, SLOT(plotSelected()));
	contextMenu.addAction(&acPlot);

	QAction acReset("Reset view", this);
	connect(&acReset, &QAction::triggered, [=]() {this->resetView(); });
	contextMenu.addAction(&acReset);

	QAction acResetAll("Reset all views", this);
	connect(&acResetAll, SIGNAL(triggered()), this, SIGNAL(resetViewsRequested()));
	contextMenu.addAction(&acResetAll);

	QAction acClip("Clip", this);
	connect(&acClip, SIGNAL(triggered()), this, SLOT(copyToClipboard()));
	contextMenu.addAction(&acClip);

	contextMenu.exec(mapToGlobal(pos));
}


void PlotWidget::copyToClipboard()
{
	QApplication::clipboard()->setPixmap(this->grab());
}


void PlotWidget::dragEnterEvent(QDragEnterEvent* event)
{
	event->acceptProposedAction();
}


void PlotWidget::dropEvent(QDropEvent* event)
{
	// Datasets are dragged into this plot.
	// If CTRL is pressed, add them to the existing datasets.
	// Otherwise, replace the datasets by the dragged ones.

	if (QApplication::keyboardModifiers() & Qt::ControlModifier)
	{
		this->addDatasets(m_DataPanel->getSelectedDatasets());
	}
	else
	{
		this->plotSelected();
	}
}

void PlotWidget::enforceAxisLimits()
{
	// X axis lower bound
	if (this->xAxis->range().lower < m_TimeRange.lower)
		this->xAxis->setRange(m_TimeRange.lower, this->xAxis->range().upper);
	// X axis upper bound
	if (this->xAxis->range().upper > m_TimeRange.upper)
		this->xAxis->setRange(this->xAxis->range().lower, m_TimeRange.upper);
	// Y axis lower bound
	if (this->yAxis->range().lower < m_DataRange.lower)
		this->yAxis->setRange(m_DataRange.lower, this->yAxis->range().upper);
	// Y axis upper bound
	if (this->yAxis->range().upper > m_DataRange.upper)
		this->yAxis->setRange(this->yAxis->range().lower, m_DataRange.upper);
}

void PlotWidget::resetView()
{
	this->xAxis->setRange(m_TimeRange);
	this->yAxis->setRange(m_DataRange);
	this->replot();
}