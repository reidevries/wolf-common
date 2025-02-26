#include "Widget.hpp"
#include "Window.hpp"

#include "Graph.hpp"
#include "ObjectPool.hpp"
#include "GraphWidget.hpp"
#include "GraphNode.hpp"
#include "Mathf.hpp"
#include "Config.hpp"

#include "Fonts/chivo_italic.hpp"

#include <chrono>
#include <cstdlib>
#include <cmath>

START_NAMESPACE_DISTRHO

const char *graphDefaultState = "0x0p+0,0x0p+0,0x0p+0,0;0x1p+0,0x1p+0,0x0p+0,0;";

GraphWidget::GraphWidget(UI *ui, Size<uint> size) : NanoWidget((NanoWidget *)ui),
                                                    fMargin(16, 16, 16, 16)
{
    setSize(size);

    const float graphWidth = size.getWidth() - fMargin.left - fMargin.right;
    const float graphHeight = size.getHeight() - fMargin.top - fMargin.bottom;

    const Size<uint> graphInnerSize = Size<uint>(graphWidth, graphHeight);

    fGraphWidgetInner = new GraphWidgetInner(ui, graphInnerSize);
    fGraphWidgetInner->parent = this;
}

GraphWidget::~GraphWidget()
{
}

void GraphWidget::onNanoDisplay()
{
    const float width = getWidth();
    const float height = getHeight();

    beginPath();

    fillColor(CONFIG_NAMESPACE::graph_margin);
    strokeColor(CONFIG_NAMESPACE::side_borders);
    strokeWidth(1.0f);

    rect(0.f, 0.f, width, height);

    fill();
    stroke();

    closePath();

    beginPath();

    const float topBorderWidth = 2.0f;

    strokeColor(CONFIG_NAMESPACE::top_border);
    strokeWidth(topBorderWidth);

    moveTo(0, 1);
    lineTo(width, 1);

    stroke();

    closePath();

    translate(fMargin.left, fMargin.top);
    fGraphWidgetInner->setAbsolutePos(getAbsoluteX() + fMargin.left, getAbsoluteY() + fMargin.top);

    fGraphWidgetInner->drawBackground();
    fGraphWidgetInner->drawGrid();
    fGraphWidgetInner->drawInOutLabels();

    fGraphWidgetInner->flipYAxis();

    if (fGraphWidgetInner->focusedElement != nullptr && dynamic_cast<GraphVertex *>(fGraphWidgetInner->focusedElement))
        fGraphWidgetInner->drawAlignmentLines();

    fGraphWidgetInner->drawGradient();
    fGraphWidgetInner->drawGraphLine(CONFIG_NAMESPACE::graph_edges_stroke_width, CONFIG_NAMESPACE::graph_edges_foreground_normal, CONFIG_NAMESPACE::graph_edges_foreground_focused); //inner

    fGraphWidgetInner->drawInputIndicator();

    if (!fGraphWidgetInner->mustHideVertices)
        fGraphWidgetInner->drawVertices();
}

void GraphWidget::onResize(const ResizeEvent &ev)
{
    if (ev.oldSize.isNull())
        return;

    const float graphInnerWidth = getWidth() - fMargin.left - fMargin.right;
    const float graphInnerHeight = getHeight() - fMargin.top - fMargin.bottom;

    const Size<uint> graphInnerSize = Size<uint>(graphInnerWidth, graphInnerHeight);

    fGraphWidgetInner->setSize(graphInnerSize);
}

void GraphWidget::rebuildFromString(const char *serializedGraph)
{
    fGraphWidgetInner->rebuildFromString(serializedGraph);
}

void GraphWidget::reset()
{
    fGraphWidgetInner->reset();
}

void GraphWidget::updateInput(const float input)
{
    fGraphWidgetInner->updateInput(input);
}

void GraphWidget::setGraphGradientMode(GraphGradientMode graphGradientMode)
{
    fGraphWidgetInner->setGraphGradientMode(graphGradientMode);
}

void GraphWidget::setHorizontalWarpAmount(const float warpAmount)
{
    fGraphWidgetInner->setHorizontalWarpAmount(warpAmount);
}

void GraphWidget::setHorizontalWarpType(const wolf::WarpType warpType)
{
    fGraphWidgetInner->setHorizontalWarpType(warpType);
}

void GraphWidget::setVerticalWarpAmount(const float warpAmount)
{
    fGraphWidgetInner->setVerticalWarpAmount(warpAmount);
}

void GraphWidget::setVerticalWarpType(const wolf::WarpType warpType)
{
    fGraphWidgetInner->setVerticalWarpType(warpType);
}

void GraphWidget::setMustHideVertices(const bool hide)
{
    fGraphWidgetInner->setMustHideVertices(hide);
}

GraphWidgetInner::GraphWidgetInner(UI *ui, Size<uint> size)
    : NanoWidget((NanoWidget *)ui),
      ui(ui),
      graphVerticesPool(wolf::maxVertices, this, GraphVertexType::Middle),
      focusedElement(nullptr),
      mouseLeftDown(false),
      mouseRightDown(false),
      graphGradientMode(GraphGradientMode::None),
      mustHideVertices(false),
      hovered(false),
      maxInput(0.0f),
      fInput(0.0f),
      fLastCurveTypeSelected(wolf::SingleCurve)
{
    setSize(size);

    initializeDefaultVertices();

    getParentWindow().addIdleCallback(this);

    fRightClickMenu = new MenuWidget(this);

    fRightClickMenu->addSection("Node");
    fRightClickMenu->addItem(deleteNodeItem, "Delete", "(double L-click)");

    fRightClickMenu->addSection("Curve Type");
    fRightClickMenu->addItem(singlePowerCurveItem, "Single Power");
    fRightClickMenu->addItem(doublePowerCurveItem, "Double Power");
    fRightClickMenu->addItem(stairsCurveItem, "Stairs");
    fRightClickMenu->addItem(waveCurveItem, "Wave");

    fRightClickMenu->setCallback(this);

    using namespace WOLF_FONTS;
    createFontFromMemory("chivo_italic", (const uchar *)chivo_italic, chivo_italic_size, 0);
}

GraphWidgetInner::~GraphWidgetInner()
{
    for (int i = 0; i < lineEditor.getVertexCount(); ++i)
    {
        delete graphVertices[i];
    }
}

void GraphWidgetInner::onResize(const ResizeEvent &ev)
{
    if (ev.oldSize.isNull())
        return;

    positionGraphNodes();
}

void GraphWidgetInner::positionGraphNodes()
{
    for (int i = 0; i < lineEditor.getVertexCount(); ++i)
    {
        GraphVertex *vertexWidget = graphVertices[i];
        wolf::Vertex *logicalVertex = lineEditor.getVertexAtIndex(i);

        vertexWidget->setPos(logicalVertex->getX() * getWidth(), logicalVertex->getY() * getHeight());
    }

    repaint();
}

void GraphWidgetInner::initializeDefaultVertices()
{
    //Left vertex
    GraphVertex *vertex = graphVerticesPool.getObject();

    vertex->setPos(0, 0);
    vertex->index = 0;
    vertex->type = GraphVertexType::Left;

    graphVertices[0] = vertex;

    //Right vertex
    vertex = graphVerticesPool.getObject();

    vertex->setPos(getWidth(), getHeight());
    vertex->index = 1;
    vertex->type = GraphVertexType::Right;

    graphVertices[1] = vertex;
}

void GraphWidgetInner::reset()
{
    resetVerticesPool();

    initializeDefaultVertices();

    ui->setState("graph", graphDefaultState);
    lineEditor.rebuildFromString(graphDefaultState);
}

void GraphWidgetInner::resetVerticesPool()
{
    for (int i = 0; i < lineEditor.getVertexCount(); ++i)
    {
        graphVerticesPool.freeObject(graphVertices[i]);
    }
}

void GraphWidgetInner::rebuildFromString(const char *serializedGraph)
{
    resetVerticesPool();

    lineEditor.rebuildFromString(serializedGraph);

    //position ui vertices to match the new graph
    for (int i = 0; i < lineEditor.getVertexCount(); ++i)
    {
        GraphVertex *vertex = graphVerticesPool.getObject();

        vertex->index = i;

        if (i == 0)
            vertex->type = GraphVertexType::Left;
        else if (i == lineEditor.getVertexCount() - 1)
            vertex->type = GraphVertexType::Right;
        else
            vertex->type = GraphVertexType::Middle;

        graphVertices[i] = vertex;
    }

    positionGraphNodes();
}

void GraphWidgetInner::updateAnimations()
{
}

void GraphWidgetInner::flipYAxis()
{
    transform(1.0f, 0.0f, 0.0f, -1.0f, 0.0f, getHeight());
}

void GraphWidgetInner::drawSubGrid()
{
}

void GraphWidgetInner::drawGrid()
{
    const float width = getWidth();
    const float height = getHeight();

    const float lineWidth = 1.0f;

    const int squaresPerRow = 8.0f;
    const int gridMiddleLineIndex = squaresPerRow / 2.0f;

    const float verticalStep = width / squaresPerRow;
    const float horizontalStep = height / squaresPerRow;

    const Color gridForegroundColor = CONFIG_NAMESPACE::grid_foreground;
    const Color gridBackgroundColor = CONFIG_NAMESPACE::grid_background;
    const Color subGridColor = CONFIG_NAMESPACE::sub_grid;
    const Color gridMiddleLineHorizontalColor = CONFIG_NAMESPACE::grid_middle_line_horizontal;
    const Color gridMiddleLineVerticalColor = CONFIG_NAMESPACE::grid_middle_line_vertical;

    //vertical
    for (int i = 0; i < squaresPerRow + 1; ++i)
    {
        const float posX = std::round(i * verticalStep);

        //subgrid
        beginPath();
        strokeWidth(lineWidth);
        strokeColor(subGridColor);

        moveTo(std::round(posX + verticalStep / 2.0f), 0.0f);
        lineTo(std::round(posX + verticalStep / 2.0f), height);

        stroke();
        closePath();

        //background
        beginPath();
        strokeWidth(lineWidth);
        strokeColor(gridBackgroundColor);

        moveTo(posX + lineWidth, 0.0f);
        lineTo(posX + lineWidth, height);

        stroke();
        closePath();

        //foreground
        beginPath();
        strokeWidth(lineWidth);
        strokeColor(i == gridMiddleLineIndex ? gridMiddleLineVerticalColor : gridForegroundColor);

        moveTo(posX, 0.0f);
        lineTo(posX, height);

        stroke();
        closePath();
    }

    //horizontal
    for (int i = 0; i < squaresPerRow + 1; ++i)
    {
        const float posY = std::round(i * horizontalStep);

        //subgrid
        beginPath();
        strokeWidth(lineWidth);
        strokeColor(subGridColor);

        moveTo(0.0f, std::round(posY + horizontalStep / 2.0f));
        lineTo(width, std::round(posY + horizontalStep / 2.0f));

        stroke();
        closePath();

        //background
        beginPath();
        strokeWidth(lineWidth);

        moveTo(0.0f, posY + lineWidth);
        lineTo(width, posY + lineWidth);

        strokeColor(gridBackgroundColor);

        stroke();
        closePath();

        //foreground
        beginPath();
        strokeWidth(lineWidth);

        moveTo(0.0f, posY);
        lineTo(width, posY);

        strokeColor(i == gridMiddleLineIndex ? gridMiddleLineHorizontalColor : gridForegroundColor);

        stroke();
        closePath();
    }
}

void GraphWidgetInner::drawBackground()
{
    const float width = getWidth();
    const float height = getHeight();

    //const float centerX = width / 2.0f;
    //const float centerY = height / 2.0f;

    beginPath();

    rect(0.f, 0.f, width, height);
    //Paint gradient = radialGradient(centerX, centerY, 1.0f, centerX, Color(42, 42, 42, 255), Color(33, 32, 39, 255));
    //fillPaint(gradient);
    fillColor(CONFIG_NAMESPACE::graph_background);
    fill();

    closePath();
}

bool GraphWidgetInner::edgeMustBeEmphasized(int vertexIndex)
{
    if (focusedElement == nullptr)
        return false;

    GraphVertex *vertex = graphVertices[vertexIndex];

    if (dynamic_cast<GraphTensionHandle *>(focusedElement))
        return focusedElement == vertex->getTensionHandle();

    return false; //might not be a good idea to make it emphasized otherwise

    /*
    if (type == GraphVertexType::Right)
        return false; //there is no edge at the right of the last vertex

    return focusedElement == vertex || focusedElement == vertex->getVertexAtRight();*/
}

void GraphWidgetInner::drawGraphEdge(int vertexIndex, float lineWidth, Color color)
{
    DISTRHO_SAFE_ASSERT(vertexIndex < lineEditor.getVertexCount() - 1);

    const float width = getWidth();
    const float height = getHeight();

    wolf::Vertex *leftVertex = lineEditor.getVertexAtIndex(vertexIndex);
    wolf::Vertex *rightVertex = lineEditor.getVertexAtIndex(vertexIndex + 1);

    beginPath();

    strokeColor(color);
    strokeWidth(lineWidth);
    lineJoin(ROUND);

    moveTo(leftVertex->getX() * width, leftVertex->getY() * height);

    const float edgeLength = (rightVertex->getX() - leftVertex->getX()) * width;

    for (int i = 0; i <= edgeLength; ++i)
    {
        const float normalizedX = leftVertex->getX() + i / width;

        lineTo(normalizedX * width, lineEditor.getValueAt(normalizedX) * height);
    }

    lineTo(rightVertex->getX() * width, rightVertex->getY() * height);

    stroke();

    closePath();
}

void GraphWidgetInner::drawGraphLine(float lineWidth, Color normalColor, Color emphasizedColor)
{
    for (int i = 0; i < lineEditor.getVertexCount() - 1; ++i)
    {
        const Color color = edgeMustBeEmphasized(i) ? emphasizedColor : normalColor;

        drawGraphEdge(i, lineWidth, color);
    }
}

void GraphWidgetInner::drawAlignmentLines()
{
    const int x = focusedElement->getX();
    const int y = focusedElement->getY();
    const int width = getWidth();
    const int height = getHeight();

    translate(0.5f, 0.5f);

    beginPath();

    strokeWidth(1.0f);
    strokeColor(CONFIG_NAMESPACE::alignment_lines);

    moveTo(x, 0);
    lineTo(x, height);

    moveTo(0, y);
    lineTo(width, y);

    stroke();

    closePath();

    translate(-0.5f, -0.5f);
}

void GraphWidgetInner::drawGradient()
{
    if (graphGradientMode == GraphGradientMode::None)
        return;

    const float width = getWidth();
    const float height = getHeight();

    float peak = 0.0f;

    beginPath();

    moveTo(0, lineEditor.getVertexAtIndex(0)->getY() * height);

    for (int vertexIndex = 0; vertexIndex < lineEditor.getVertexCount() - 1; ++vertexIndex)
    {
        wolf::Vertex *leftVertex = lineEditor.getVertexAtIndex(vertexIndex);
        wolf::Vertex *rightVertex = lineEditor.getVertexAtIndex(vertexIndex + 1);

        const float edgeLength = (rightVertex->getX() - leftVertex->getX()) * width;

        for (int i = 0; i <= edgeLength; ++i)
        {
            const float normalizedX = leftVertex->getX() + i / width;
            const float x = normalizedX * width;
            const float y = lineEditor.getValueAt(normalizedX) * height;

            lineTo(x, y);

            peak = std::max(peak, y);
        }
    }

    lineTo(width, 0);
    lineTo(0, 0);

    fillPaint(linearGradient(width / 2.0f, 0, width / 2.0f, peak, CONFIG_NAMESPACE::graph_gradient_icol, CONFIG_NAMESPACE::graph_gradient_ocol));
    fill();

    closePath();
}

void GraphWidgetInner::updateInput(const float input)
{
    fInput = input;
}

void GraphWidgetInner::setGraphGradientMode(GraphGradientMode graphGradientMode)
{
    this->graphGradientMode = graphGradientMode;
    repaint();
}

void GraphWidgetInner::setHorizontalWarpAmount(const float warpAmount)
{
    lineEditor.setHorizontalWarpAmount(warpAmount);
    positionGraphNodes();
}

void GraphWidgetInner::setHorizontalWarpType(const wolf::WarpType warpType)
{
    lineEditor.setHorizontalWarpType(warpType);
    positionGraphNodes();
}

void GraphWidgetInner::setVerticalWarpAmount(const float warpAmount)
{
    lineEditor.setVerticalWarpAmount(warpAmount);
    positionGraphNodes();
}

void GraphWidgetInner::setVerticalWarpType(const wolf::WarpType warpType)
{
    lineEditor.setVerticalWarpType(warpType);
    positionGraphNodes();
}

void GraphWidgetInner::setMustHideVertices(const bool hide)
{
    mustHideVertices = hide;
    repaint();
}

void GraphWidgetInner::drawInputIndicator()
{
    const float width = getWidth();
    const float height = getHeight();

    if (fInput <= 0.0f)
        return;

    const float inputIndicatorX = std::round(fInput * width);

    translate(0.5f, 0.5f);

    beginPath();

    strokeColor(CONFIG_NAMESPACE::input_volume_indicator);
    strokeWidth(1.0f);

    moveTo(inputIndicatorX, 0);
    lineTo(inputIndicatorX, height);

    stroke();

    closePath();

    translate(-0.5f, -0.5f);

    //circle at indicator intersection with the graph
    const float circleY = lineEditor.getValueAt(fInput) * height;

    beginPath();

    fillColor(CONFIG_NAMESPACE::playhead_circle_fill);
    strokeColor(CONFIG_NAMESPACE::playhead_circle_stroke);

    circle(inputIndicatorX, circleY, 3.5f);
    fill();
    stroke();
    closePath();
}

void GraphWidgetInner::idleCallback()
{
    repaint();
}

void GraphWidgetInner::drawInOutLabels()
{
    fontFace("chivo_italic");
    fontSize(36.f);
    fillColor(255, 255, 255, 125);

    textAlign(ALIGN_BOTTOM | ALIGN_RIGHT);
    text(getWidth() - 5, getHeight(), "In", NULL);

    textAlign(ALIGN_TOP | ALIGN_LEFT);
    text(5, 0, "Out", NULL);
}

void GraphWidgetInner::drawVertices()
{
    const int vertexCount = lineEditor.getVertexCount();

    for (int i = 0; i < vertexCount; ++i)
    {
        GraphVertex *vertex = graphVertices[i];

        vertex->getTensionHandle()->render();
        vertex->render();
    }
}

void GraphWidgetInner::onNanoDisplay()
{
}

bool GraphWidgetInner::onScroll(const ScrollEvent &ev)
{
    const Point<int> point = wolf::flipY(ev.pos, getHeight());

    //Testing for mouse hover on tension handles
    for (int i = 0; i < lineEditor.getVertexCount() - 1; ++i)
    {
        GraphTensionHandle *tensionHandle = graphVertices[i]->getTensionHandle();

        if (tensionHandle->contains(point))
        {
            const float delta = graphVertices[i]->getY() < graphVertices[i + 1]->getY() ? -ev.delta.getY() : ev.delta.getY();
            const float oldTension = lineEditor.getVertexAtIndex(i)->getTension();

            lineEditor.setTensionAtIndex(i, wolf::clamp(oldTension + 1.5f * delta, -100.0f, 100.0f));

            ui->setState("graph", lineEditor.serialize());
            repaint();

            getParentWindow().setCursorPos(tensionHandle->getAbsoluteX(), tensionHandle->getAbsoluteY());

            return true;
        }
    }

    return false;
}

void GraphWidgetInner::removeVertex(int index)
{
    //Make sure the vertex to remove is in the middle of the graph
    if (index <= 0)
        return;
    else if (index >= lineEditor.getVertexCount() - 1)
        return;

    //Get rid of the ui widget
    graphVerticesPool.freeObject(graphVertices[index]);

    const int vertexCount = lineEditor.getVertexCount() - 1;

    for (int i = index; i < vertexCount; ++i)
    {
        graphVertices[i] = graphVertices[i + 1];
        graphVertices[i]->index--;
    }

    //Get rid of the logical vertex and update dsp
    lineEditor.removeVertex(index);
    ui->setState("graph", lineEditor.serialize());

    focusedElement = nullptr;

    repaint();
}

GraphVertex *GraphWidgetInner::insertVertex(const Point<int> pos)
{
    int i = lineEditor.getVertexCount();

    if (i == wolf::maxVertices)
        return nullptr;

    while ((i > 0) && (pos.getX() < graphVertices[i - 1]->getX()))
    {
        graphVertices[i] = graphVertices[i - 1];
        graphVertices[i]->index++;

        --i;
    }

    GraphVertex *vertex = graphVerticesPool.getObject();

    vertex->index = i;

    graphVertices[i] = vertex;

    const float width = getWidth();
    const float height = getHeight();

    const float normalizedX = wolf::normalize(pos.getX(), width);
    const float normalizedY = wolf::normalize(pos.getY(), height);

    lineEditor.insertVertex(normalizedX, normalizedY, 0, fLastCurveTypeSelected);

    ui->setState("graph", lineEditor.serialize());

    positionGraphNodes();

    return vertex;
}

GraphNode *GraphWidgetInner::getHoveredNode(Point<int> cursorPos)
{
    //Testing for mouse hover on graph vertices
    for (int i = lineEditor.getVertexCount() - 1; i >= 0; --i)
    {
        if (graphVertices[i]->contains(cursorPos))
        {
            return graphVertices[i];
        }
    }

    //Testing for mouse hover on tension handles
    for (int i = lineEditor.getVertexCount() - 1; i >= 0; --i)
    {
        if (graphVertices[i]->tensionHandle.contains(cursorPos))
        {
            return &graphVertices[i]->tensionHandle;
        }
    }

    return nullptr;
}

bool GraphWidgetInner::leftClick(const MouseEvent &ev)
{
    const Point<int> point = wolf::flipY(ev.pos, getHeight());

    if (mouseRightDown)
        return true;

    mouseLeftDown = ev.press;

    if (!ev.press)
    {
        if (focusedElement != nullptr)
        {
            focusedElement->onMouse(ev);
            focusedElement = nullptr;
        }

        return true;
    }

    GraphNode *hoveredNode = getHoveredNode(point);

    if (hoveredNode != nullptr)
    {
        focusedElement = hoveredNode;

        return focusedElement->onMouse(ev);
    }

    //The cursor is not over any graph node
    return false;
}

bool GraphWidgetInner::middleClick(const MouseEvent &)
{
    return false;
}

void GraphWidgetInner::menuItemSelected(const int id)
{
    GraphVertex *vertex = static_cast<GraphVertex *>(fNodeSelectedByRightClick);

    if (id == deleteNodeItem)
    {
        removeVertex(vertex->getIndex());
    }
    else
    {
        wolf::CurveType type = (wolf::CurveType)(id - 1);

        lineEditor.getVertexAtIndex(vertex->getIndex())->setType(type);
        fLastCurveTypeSelected = type;

        ui->setState("graph", lineEditor.serialize());
        repaint();
    }
}

void GraphWidgetInner::propagateMouseEvent(const MouseEvent &ev)
{
	switch (ev.button) {
		case 1:
			mouseLeftDown = ev.press;
			break;
		case 3:
			mouseRightDown = ev.press;
			break;
	}
}

bool GraphWidgetInner::rightClick(const MouseEvent &ev)
{
    const Point<int> point = wolf::flipY(ev.pos, getHeight());

    if (mouseLeftDown)
        return true;

    mouseRightDown = ev.press;

    if (focusedElement == nullptr)
    {
        GraphNode *node = getHoveredNode(point);

        if (node != nullptr)
        {
            //check if tension handle, if true, reset tension
            GraphTensionHandle *tensionHandle = dynamic_cast<GraphTensionHandle *>(node);

            if (tensionHandle != nullptr)
            {
                tensionHandle->reset();

                //if the handle is not on the cursor anymore, we reset the cursor style
                node = getHoveredNode(point);

                if (node == nullptr)
                {
                    getParentWindow().setCursorStyle(Window::CursorStyle::Default);
                }

                repaint();
            }

            //else, show curve selection menu
            else
            {
                fNodeSelectedByRightClick = node;

				// disable certain items depending which kind of vertex selected
				fRightClickMenu->setAllItemsEnabled(true);
				const auto vertex = dynamic_cast<GraphVertex*>(node);
				const auto vertex_type = vertex->getType();
				if (vertex_type != GraphVertexType::Middle) {
					fRightClickMenu->setItemEnabled(section_index_delete, false);
					if (vertex_type == GraphVertexType::Right) {
						fRightClickMenu->setItemEnabled(section_index_curve, false);
					}
				}

				// set the currently selected curve type in the menu
				const wolf::CurveType vertex_curve = lineEditor
					.getVertexAtIndex(vertex->getIndex())
					->getType();
				fRightClickMenu->setItemSelected(vertex_curve+3);


				// get click position and the bounds of this widget
				auto click_pos = Point<int>(
					getAbsoluteX() + ev.pos.getX(),
					getAbsoluteY() + ev.pos.getY()
				);
				auto widget_bounds = Rectangle<int>(
					getAbsoluteX(), getAbsoluteY(),
					getWidth(), getHeight()
				);
				fRightClickMenu->show(click_pos, widget_bounds);
            }

            return true;
        }

        if (ev.press && contains(ev.pos))
        {
            focusedElement = insertVertex(point);

            if (focusedElement != nullptr)
                return focusedElement->onMouse(ev);
        }
    }
    else
    {
        focusedElement->onMouse(ev);
        focusedElement = nullptr;

        return true;
    }

    return false;
}

bool GraphWidgetInner::onMouse(const MouseEvent &ev)
{
    if (mustHideVertices)
        return false;

    switch (ev.button)
    {
    case 1:
        return leftClick(ev);
    case 2:
        return middleClick(ev);
    case 3:
        return rightClick(ev);
    }

    return false;
}

bool GraphWidgetInner::onMotion(const MotionEvent &ev)
{
    if (mustHideVertices)
        return false;

    const Point<int> point = wolf::flipY(ev.pos, getHeight());
    GraphNode *hoveredNode = getHoveredNode(point);

    if (contains(ev.pos) || hoveredNode != nullptr)
    {
        hovered = true;
    }
    else if (hovered && !contains(ev.pos) && focusedElement == nullptr)
    {
        onMouseLeave();
        hovered = false;

        return false;
    }
    else if (focusedElement == nullptr)
    {
        return false;
    }

    if (focusedElement != nullptr)
    {
        return focusedElement->onMotion(ev);
    }

    if (hoveredNode != nullptr)
    {
        return hoveredNode->onMotion(ev);
    }

    //The mouse pointer is not over any graph node
    getParentWindow().setCursorStyle(Window::CursorStyle::Default);

    return true;
}

void GraphWidgetInner::onFocusOut()
{
    if (focusedElement != nullptr)
    {
        focusedElement->grabbed = false;
        focusedElement = nullptr;
    }

    hovered = false;
    mouseLeftDown = false;
    mouseRightDown = false;

    getParentWindow().showCursor();

    repaint();
}

void GraphWidgetInner::onMouseLeave()
{
    getParentWindow().setCursorStyle(Window::CursorStyle::Default);
}

END_NAMESPACE_DISTRHO
