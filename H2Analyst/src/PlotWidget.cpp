#include "PlotWidget.h"


PlotWidget::PlotWidget(QWidget* parent) : QCustomPlot(parent),
m_DataPanel(nullptr),
m_Datasets(),
m_TimeRange(0.0, 10.0),
m_DataRange(0.0, 5.0),
m_MaxTimePadding(0.0),
m_MaxDataPadding(0.0),
m_CrosshairEnabled(false),
m_CrosshairH(nullptr),
m_CrosshairV(nullptr),
m_CrosshairLabelX(nullptr),
m_CrosshairLabelY(nullptr),
m_MousePos(0, 0)
{
	this->setMouseTracking(true); // Should be default for QCustomPlot, but to be sure

	// Add layer for crosshair and create crosshair elements (lines and labels)
	if (this->addLayer("cursor", this->layer("axes"))) { }
	else { std::cout << "[Error] Failed to add cursor layer to plot" << std::endl; }
	this->setCurrentLayer("cursor");
	this->layer("cursor")->setVisible(false);
	this->layer("cursor")->setMode(QCPLayer::lmBuffered);
	m_CrosshairH = new QCPItemLine(this);
	m_CrosshairV = new QCPItemLine(this);
	m_CrosshairLabelX = new QCPItemText(this);
	m_CrosshairLabelX->setClipToAxisRect(false);
	m_CrosshairLabelX->setBrush(QBrush(Qt::white));
	m_CrosshairLabelX->setPen(QPen(Qt::black));
	m_CrosshairLabelX->setPadding(QMargins(2, 2, 2, 2));
	m_CrosshairLabelX->setPositionAlignment(Qt::AlignBottom | Qt::AlignHCenter);
	m_CrosshairLabelX->position->setType(QCPItemPosition::ptPlotCoords);
	m_CrosshairLabelY = new QCPItemText(this);
	m_CrosshairLabelY->setClipToAxisRect(false);
	m_CrosshairLabelY->setBrush(QBrush(Qt::white));
	m_CrosshairLabelY->setPen(QPen(Qt::black));
	m_CrosshairLabelY->setPadding(QMargins(2, 2, 2, 2));
	m_CrosshairLabelY->setPositionAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	m_CrosshairLabelY->position->setType(QCPItemPosition::ptPlotCoords);
	this->setCurrentLayer("main");

	// Set default ranges and label
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
	m_DataRange = QCPRange(ymin, ymax);
	if (m_DataRange.lower > 0.0) m_DataRange.lower = 0.0; // This (almost) always makes sense for visualization
	m_MaxTimePadding = (m_TimeRange.upper - m_TimeRange.lower) * RANGE_PADDING_X;
	m_MaxDataPadding = (m_DataRange.upper - m_DataRange.lower) * RANGE_PADDING_Y;

	this->resetView();
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

// Function that makes sure the axis ranges stay within limits to keep data in view
void PlotWidget::enforceAxisLimits()
{
	// Determine padding as fraction of visible range (to keep the screen space padding constant)
	double xPadding = (this->xAxis->range().upper - this->xAxis->range().lower) * RANGE_PADDING_X;
	double yPadding = (this->yAxis->range().upper - this->yAxis->range().lower) * RANGE_PADDING_Y;
	// Make sure this padding is not larger than the absolute max based on the data itself
	xPadding = std::min({ xPadding, m_MaxTimePadding });
	yPadding = std::min({ yPadding, m_MaxDataPadding });

	// X axis lower bound
	if (this->xAxis->range().lower < m_TimeRange.lower - xPadding)
		this->xAxis->setRange(m_TimeRange.lower - xPadding, this->xAxis->range().upper);
	// X axis upper bound
	if (this->xAxis->range().upper > m_TimeRange.upper + xPadding)
		this->xAxis->setRange(this->xAxis->range().lower, m_TimeRange.upper + xPadding);
	// Y axis lower bound
	if (this->yAxis->range().lower < m_DataRange.lower - yPadding)
		this->yAxis->setRange(m_DataRange.lower - yPadding, this->yAxis->range().upper);
	// Y axis upper bound
	if (this->yAxis->range().upper > m_DataRange.upper + yPadding)
		this->yAxis->setRange(this->yAxis->range().lower, m_DataRange.upper + yPadding);
}

// Function to reset view to fit all data with padding included
void PlotWidget::resetView()
{
	this->xAxis->setRange(m_TimeRange.lower - m_MaxTimePadding, m_TimeRange.upper + m_MaxTimePadding);
	this->yAxis->setRange(m_DataRange.lower - m_MaxDataPadding, m_DataRange.upper + m_MaxDataPadding);
	this->replot();
}

void PlotWidget::leaveEvent(QEvent* event)
{
	// Should be done in mouseMoveEvent, but to be sure
	m_CrosshairEnabled = false;
	this->layer("cursor")->setVisible(false);
	this->layer("cursor")->replot();

	QCustomPlot::leaveEvent(event);
}

void PlotWidget::mouseMoveEvent(QMouseEvent* event)
{
	m_MousePos = event->pos();

	this->updateCrosshair();
	
	// If mouse is in plot area (within axis ranges), hide cursor and show crosshair
	if (this->xAxis->range().lower < this->xAxis->pixelToCoord(m_MousePos.x()) &&
		this->xAxis->pixelToCoord(m_MousePos.x()) < this->xAxis->range().upper &&
		this->yAxis->range().lower < this->yAxis->pixelToCoord(m_MousePos.y()) &&
		this->yAxis->pixelToCoord(m_MousePos.y()) < this->yAxis->range().upper)
	{
		this->setCursor(Qt::BlankCursor);
		m_CrosshairEnabled = true;
		this->layer("cursor")->setVisible(true);
	}
	else
	{
		this->setCursor(Qt::ArrowCursor);
		m_CrosshairEnabled = false;
		this->layer("cursor")->setVisible(false);
		this->layer("cursor")->replot();
	}


	if (!this->isEmpty())
		QCustomPlot::mouseMoveEvent(event);
}

void PlotWidget::wheelEvent(QWheelEvent* event)
{
	if (!this->isEmpty())
		QCustomPlot::wheelEvent(event);
	this->updateCrosshair();
}

void PlotWidget::updateCrosshair()
{
	if (!m_CrosshairEnabled) return;

	double xOrigin = this->xAxis->pixelToCoord(m_MousePos.x());
	double yOrigin = this->yAxis->pixelToCoord(m_MousePos.y());
	
	// Crosshair lines
	m_CrosshairH->start->setCoords(this->xAxis->range().lower, yOrigin);
	m_CrosshairH->end->setCoords(this->xAxis->range().upper, yOrigin);
	m_CrosshairV->start->setCoords(xOrigin, this->yAxis->range().upper);
	m_CrosshairV->end->setCoords(xOrigin, this->yAxis->range().lower);

	// Labels
	m_CrosshairLabelX->position->setCoords(xOrigin, this->yAxis->range().lower);
	std::stringstream stream;
	stream << std::fixed << std::setprecision(2) << xOrigin;
	m_CrosshairLabelX->setText(QString(stream.str().c_str()));

	m_CrosshairLabelY->position->setCoords(this->xAxis->range().lower, yOrigin);
	stream.str(""); // Clear stream
	stream << std::fixed << std::setprecision(2) << yOrigin;
	m_CrosshairLabelY->setText(QString(stream.str().c_str()));

	this->layer("cursor")->replot();
}