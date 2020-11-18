/*
  ==============================================================================

    ConnectionUIHelper.cpp
    Created: 18 Nov 2020 2:48:48pm
    Author:  bkupe

  ==============================================================================
*/

#include "ConnectionUIHelper.h"

Path PathHelpers::buildHitPath(Path* sourcePath)
{
	Path hitPath;

	Array<Point<float>> firstPoints;
	Array<Point<float>> secondPoints;

	const int numPoints = 10;
	const int margin = 10;

	Array<Point<float>> points;
	for (int i = 0; i < numPoints; i++)
	{
		points.add(sourcePath->getPointAlongPath(sourcePath->getLength() * i / jmax(numPoints-1,1)));
	}

	for (int i = 0; i < numPoints; i++)
	{
		Point<float> tp;
		Point<float> sp;

		if (i == 0 || i == numPoints - 1)
		{
			tp = points[i].translated(0, -margin);
			sp = points[i].translated(0, margin);
		}
		else
		{
			float angle1 = points[i].getAngleToPoint(points[i - 1]);
			float angle2 = points[i].getAngleToPoint(points[i + 1]);

			if (angle1 < 0) angle1 += float_Pi * 2;

			if (angle2 < 0) angle2 += float_Pi * 2;

			float angle = (angle1 + angle2) / 2.f;

			if (angle1 < angle2) angle += float_Pi;

			//            DBG("Point " << i << ", angle : " << angle << " >>  " << String(angle1>angle2));

			tp = points[i].getPointOnCircumference(margin, angle + float_Pi);
			sp = points[i].getPointOnCircumference(margin, angle);
		}

		firstPoints.add(tp);
		secondPoints.insert(0, sp);
	}

	hitPath.startNewSubPath(firstPoints[0]);

	for (int i = 1; i < firstPoints.size(); i++) hitPath.lineTo(firstPoints[i]);

	for (int i = 0; i < secondPoints.size(); i++) hitPath.lineTo(secondPoints[i]);

	hitPath.closeSubPath();

	return hitPath;
}
