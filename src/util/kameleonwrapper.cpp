/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014                                                                    *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#include <ghoul/logging/logmanager.h>
#include <openspace/util/kameleonwrapper.h>

#include <ccmc/Model.h>
#include <ccmc/Interpolator.h>
#include <ccmc/BATSRUS.h>
#include <ccmc/ENLIL.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <iomanip>
#include <stdlib.h>

namespace openspace {

std::string _loggerCat = "KameleonWrapper";

KameleonWrapper::KameleonWrapper(const std::string& filename, Model model): _type(model) {
	switch (_type) {
        case Model::BATSRUS:
            _model = new ccmc::BATSRUS();
            if(!_model) LERROR("BATSRUS:Failed to create BATSRUS model instance");
            if (_model->open(filename) != ccmc::FileReader::OK)
				LERROR("BATSRUS:Failed to open "+filename);
            _interpolator = _model->createNewInterpolator();
            if (!_interpolator) LERROR("BATSRUS:Failed to create BATSRUS interpolator");
            break;
        case Model::ENLIL:
            _model = new ccmc::ENLIL();
            if(!_model) LERROR("Failed to create ENLIL model instance");
            if (_model->open(filename) != ccmc::FileReader::OK)
				LERROR("Failed to open "+filename);
            _interpolator = _model->createNewInterpolator();
            if (!_interpolator) LERROR("Failed to create ENLIL interpolator");
            break;
        default:
            LERROR("No valid model type provided!");
	}

	getGridVariables(_xCoordVar, _yCoordVar, _zCoordVar);
	_xMin =    _model->getVariableAttribute(_xCoordVar, "actual_min").getAttributeFloat();
	_xMax =    _model->getVariableAttribute(_xCoordVar, "actual_max").getAttributeFloat();
	_yMin =    _model->getVariableAttribute(_yCoordVar, "actual_min").getAttributeFloat();
	_yMax =    _model->getVariableAttribute(_yCoordVar, "actual_max").getAttributeFloat();
	_zMin =    _model->getVariableAttribute(_zCoordVar, "actual_min").getAttributeFloat();
	_zMax =    _model->getVariableAttribute(_zCoordVar, "actual_max").getAttributeFloat();

	_lastiProgress = -1; // For progressbar
}

KameleonWrapper::~KameleonWrapper() {
	delete _model;
	delete _interpolator;
}

float* KameleonWrapper::getUniformSampledValues(const std::string& var, glm::size3_t outDimensions) {
	assert(_model && _interpolator);
	assert(outDimensions.x > 0 && outDimensions.y > 0 && outDimensions.z > 0);
    assert(_type == Model::ENLIL || _type == Model::BATSRUS);
    LINFO("Loading variable " << var << " from CDF data with a uniform sampling");

	int size = outDimensions.x*outDimensions.y*outDimensions.z;
	float* data = new float[size];

	float varMin =  _model->getVariableAttribute(var, "actual_min").getAttributeFloat();
	float varMax =  _model->getVariableAttribute(var, "actual_max").getAttributeFloat();

	float stepX = (_xMax-_xMin)/(static_cast<float>(outDimensions.x));
	float stepY = (_yMax-_yMin)/(static_cast<float>(outDimensions.y));
	float stepZ = (_zMax-_zMin)/(static_cast<float>(outDimensions.z));
    
    LDEBUG(var << "Min: " << varMin);
    LDEBUG(var << "Max: " << varMax);
    
    for (int x = 0; x < outDimensions.x; ++x) {
    	progressBar(x, outDimensions.x);
        
		for (int y = 0; y < outDimensions.y; ++y) {
			for (int z = 0; z < outDimensions.z; ++z) {
                
                int index = x + y*outDimensions.x + z*outDimensions.x*outDimensions.y;
                
                if(_type == Model::BATSRUS) {
                    float xPos = _xMin + stepX*x;
                    float yPos = _yMin + stepY*y;
                    float zPos = _zMin + stepZ*z;
                    
                    // get interpolated data value for (xPos, yPos, zPos)
                    float value = _interpolator->interpolate(var, xPos, yPos, zPos);
                    
                    // scale to [0,1]
                    data[index] = (value-varMin)/(varMax-varMin);
                } else if (_type == Model::ENLIL) {
                    
                    // Put r in the [0..sqrt(3)] range
                    float rNorm = sqrt(3.0)*(float)x/(float)(outDimensions.x-1);
                    
                    // Put theta in the [0..PI] range
                    float thetaNorm = M_PI*(float)y/(float)(outDimensions.y-1);
                    
                    // Put phi in the [0..2PI] range
                    float phiNorm = 2.0*M_PI*(float)z/(float)(outDimensions.z-1);
                    
                    // Go to physical coordinates before sampling
                    float rPh = _xMin + rNorm*(_xMax-_xMin);
                    float thetaPh = thetaNorm;
                    // phi range needs to be mapped to the slightly different model
                    // range to avoid gaps in the data Subtract a small term to
                    // avoid rounding errors when comparing to phiMax.
                    float phiPh = _zMin + phiNorm/(2.0*M_PI)*(_zMax-_zMin-0.000001);
                    
                    float varValue = 0.f;
                    // See if sample point is inside domain
                    if (rPh < _xMin || rPh > _xMax || thetaPh < _yMin ||
                        thetaPh > _yMax || phiPh < _zMin || phiPh > _zMax) {
                        if (phiPh > _zMax) {
                            std::cout << "Warning: There might be a gap in the data\n";
                        }
                        // Leave values at zero if outside domain
                    } else { // if inside
                        
                        // ENLIL CDF specific hacks!
                        // Convert from meters to AU for interpolator
                        rPh /= ccmc::constants::AU_in_meters;
                        // Convert from colatitude [0, pi] rad to latitude [-90, 90] degrees
                        thetaPh = -thetaPh*180.f/M_PI+90.f;
                        // Convert from [0, 2pi] rad to [0, 360] degrees
                        phiPh = phiPh*180.f/M_PI;
                        // Sample
                        varValue = _interpolator->interpolate(var, rPh, thetaPh, phiPh);
                    }

                    data[index] = (varValue-varMin)/(varMax-varMin);
                }
			}
		}
	}
    std::cout << std::endl;
    LINFO("Done!");

	return data;
}

float* KameleonWrapper::getUniformSampledVectorValues(const std::string& xVar, const std::string& yVar, const std::string& zVar, glm::size3_t outDimensions) {
	assert(_model && _interpolator);
	assert(outDimensions.x > 0 && outDimensions.y > 0 && outDimensions.z > 0);
	assert(_type == Model::ENLIL || _type == Model::BATSRUS);
	LINFO("Loading variables " << xVar << " " << yVar << " " << zVar << " from CDF data with a uniform sampling");

	int channels = 4;
	int size = channels*outDimensions.x*outDimensions.y*outDimensions.z;
	float* data = new float[size];

	float varXMin =  _model->getVariableAttribute(xVar, "actual_min").getAttributeFloat();
	float varXMax =  _model->getVariableAttribute(xVar, "actual_max").getAttributeFloat();
	float varYMin =  _model->getVariableAttribute(yVar, "actual_min").getAttributeFloat();
	float varYMax =  _model->getVariableAttribute(yVar, "actual_max").getAttributeFloat();
	float varZMin =  _model->getVariableAttribute(zVar, "actual_min").getAttributeFloat();
	float varZMax =  _model->getVariableAttribute(zVar, "actual_max").getAttributeFloat();

	float stepX = (_xMax-_xMin)/(static_cast<float>(outDimensions.x));
	float stepY = (_yMax-_yMin)/(static_cast<float>(outDimensions.y));
	float stepZ = (_zMax-_zMin)/(static_cast<float>(outDimensions.z));

	LDEBUG(xVar << "Min: " << varXMin);
	LDEBUG(xVar << "Max: " << varXMax);
	LDEBUG(yVar << "Min: " << varYMin);
	LDEBUG(yVar << "Max: " << varYMax);
	LDEBUG(zVar << "Min: " << varZMin);
	LDEBUG(zVar << "Max: " << varZMax);

	for (int x = 0; x < outDimensions.x; ++x) {
		progressBar(x, outDimensions.x);

		for (int y = 0; y < outDimensions.y; ++y) {
			for (int z = 0; z < outDimensions.z; ++z) {

				int index = x*channels + y*channels*outDimensions.x + z*channels*outDimensions.x*outDimensions.y;

				if(_type == Model::BATSRUS) {
					float xPos = _xMin + stepX*x;
					float yPos = _yMin + stepY*y;
					float zPos = _zMin + stepZ*z;

					// get interpolated data value for (xPos, yPos, zPos)
					float xValue = _interpolator->interpolate(xVar, xPos, yPos, zPos);
					float yValue = _interpolator->interpolate(yVar, xPos, yPos, zPos);
					float zValue = _interpolator->interpolate(zVar, xPos, yPos, zPos);

					// scale to [0,1]
					data[index] 	= (xValue-varXMin)/(varXMax-varXMin); // R
					data[index + 1] = (yValue-varYMin)/(varYMax-varYMin); // G
					data[index + 2] = (zValue-varZMin)/(varZMax-varZMin); // B
					data[index + 3] = 1.0; // GL_RGB refuses to work. Workaround by doing a GL_RGBA with hardcoded alpha
				} else {
					LERROR("Only BATSRUS supported for getUniformSampledVectorValues (for now)");
				}
			}
		}
	}
	std::cout << std::endl;
	LINFO("Done!");

	return data;
}

std::vector<std::vector<glm::vec3> > KameleonWrapper::getFieldLines(
		const std::string& xVar, const std::string& yVar,
		const std::string& zVar, std::vector<glm::vec3> seedPoints,
		float stepSize ) {
	assert(_model && _interpolator);
	assert(_type == Model::ENLIL || _type == Model::BATSRUS);
	LINFO("Creating " << seedPoints.size() << " fieldlines from variables " << xVar << " " << yVar << " " << zVar);

	std::vector<glm::vec3> fLine, bLine;
	std::vector<std::vector<glm::vec3> > fieldLines;
	glm::vec3 color;
	FieldlineEnd forwardEnd, backEnd;

	if (_type == Model::BATSRUS) {
		for (glm::vec3 seedPoint : seedPoints) {
			fLine = traceCartesianFieldline(xVar, yVar, zVar, seedPoint, stepSize, TraceDirection::FORWARD, forwardEnd);
			bLine = traceCartesianFieldline(xVar, yVar, zVar, seedPoint, stepSize, TraceDirection::BACK, backEnd);

			bLine.insert(bLine.begin(), fLine.rbegin(), fLine.rend());

			// classify
			color = classifyFieldline(forwardEnd, backEnd);

			// write colors
			std::vector<glm::vec3> line;
			for (glm::vec3 position : bLine) {
				line.push_back(position);
				line.push_back(color);
			}

			fieldLines.push_back(line);
		}
	} else {
		LERROR("Fieldlines are only supported for BATSRUS model");
	}

	return fieldLines;
}

std::vector<glm::vec3> KameleonWrapper::traceCartesianFieldline(
		const std::string& xVar, const std::string& yVar,
		const std::string& zVar, glm::vec3 seedPoint,
		float stepSize, TraceDirection direction, FieldlineEnd& end) {

	glm::vec3 color, pos, k1, k2, k3, k4;
	std::vector<glm::vec3> line;
	float stepX, stepY, stepZ;
	int numSteps = 0;
	int maxSteps = 5000;
	pos = seedPoint;

	_model->loadVariable(xVar);
	_model->loadVariable(yVar);
	_model->loadVariable(zVar);

	long int xID = _model->getVariableID(xVar);
	long int yID = _model->getVariableID(yVar);
	long int zID = _model->getVariableID(zVar);

	// While we are inside the models boundries and not inside earth
	while ((pos.x < _xMax && pos.x > _xMin && pos.y < _yMax && pos.y > _yMin &&
			pos.z < _zMax && pos.z > _zMin) && !(pos.x*pos.x + pos.y*pos.y + pos.z*pos.z < 1.0)) {

		// Save position
		line.push_back(pos);

		// Calculate new position with Runge-Kutta 4th order
		k1.x = _interpolator->interpolate(xID, pos.x, pos.y, pos.z, stepX, stepY, stepZ);
		k1.y = _interpolator->interpolate(yID, pos.x, pos.y, pos.z);
		k1.z = _interpolator->interpolate(zID, pos.x, pos.y, pos.z);
		k1 = (float)direction*glm::normalize(k1);
//		stepX*=stepSize, stepY*=stepSize, stepZ*=stepSize;
		k2.x = _interpolator->interpolate(xID, pos.x+(stepX/2.0)*k1.x, pos.y+(stepY/2.0)*k1.y, pos.z+(stepZ/2.0)*k1.z);
		k2.y = _interpolator->interpolate(yID, pos.x+(stepX/2.0)*k1.x, pos.y+(stepY/2.0)*k1.y, pos.z+(stepZ/2.0)*k1.z);
		k2.z = _interpolator->interpolate(zID, pos.x+(stepX/2.0)*k1.x, pos.y+(stepY/2.0)*k1.y, pos.z+(stepZ/2.0)*k1.z);
		k2 = (float)direction*glm::normalize(k2);
		k3.x = _interpolator->interpolate(xID, pos.x+(stepX/2.0)*k2.x, pos.y+(stepY/2.0)*k2.y, pos.z+(stepZ/2.0)*k2.z);
		k3.y = _interpolator->interpolate(yID, pos.x+(stepX/2.0)*k2.x, pos.y+(stepY/2.0)*k2.y, pos.z+(stepZ/2.0)*k2.z);
		k3.z = _interpolator->interpolate(zID, pos.x+(stepX/2.0)*k2.x, pos.y+(stepY/2.0)*k2.y, pos.z+(stepZ/2.0)*k2.z);
		k3 = (float)direction*glm::normalize(k3);
		k4.x = _interpolator->interpolate(xID, pos.x+stepX*k3.x, pos.y+stepY*k3.y, pos.z+stepZ*k3.z);
		k4.y = _interpolator->interpolate(yID, pos.x+stepX*k3.x, pos.y+stepY*k3.y, pos.z+stepZ*k3.z);
		k4.z = _interpolator->interpolate(zID, pos.x+stepX*k3.x, pos.y+stepY*k3.y, pos.z+stepZ*k3.z);
		k4 = (float)direction*glm::normalize(k4);
		pos.x = pos.x + (stepX/6.0)*(k1.x + 2.0*k2.x + 2.0*k3.x + k4.x);
		pos.y = pos.y + (stepY/6.0)*(k1.y + 2.0*k2.y + 2.0*k3.y + k4.y);
		pos.z = pos.z + (stepZ/6.0)*(k1.z + 2.0*k2.z + 2.0*k3.z + k4.z);

		++numSteps;
		if (numSteps > maxSteps) {
			LDEBUG("Max number of steps taken (" << maxSteps <<")");
			break;
		}
	}
	line.push_back(pos);

	if (pos.z > 0.0 && (pos.x*pos.x + pos.y*pos.y + pos.z*pos.z < 1.0))
		end = FieldlineEnd::NORTH;
	else if (pos.z < 0.0 && (pos.x*pos.x + pos.y*pos.y + pos.z*pos.z < 1.0))
		end = FieldlineEnd::SOUTH;
	else
		end = FieldlineEnd::OUT;

	return line;
}

void KameleonWrapper::getGridVariables(std::string& x, std::string& y, std::string& z) {
	// get the grid system string
	std::string gridSystem = _model->getGlobalAttribute("grid_system_1").getAttributeString();

	// remove leading and trailing brackets
	gridSystem = gridSystem.substr(1,gridSystem.length()-2);

	// remove all whitespaces
	gridSystem.erase(remove_if(gridSystem.begin(), gridSystem.end(), isspace), gridSystem.end());

	// replace all comma signs with whitespaces
	std::replace( gridSystem.begin(), gridSystem.end(), ',', ' ');

	// tokenize
	std::istringstream iss(gridSystem);
	std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},std::istream_iterator<std::string>{}};

	// validate
	if (tokens.size() != 3) LERROR("Something went wrong");

	x = tokens.at(0);
	y = tokens.at(1);
	z = tokens.at(2);
}

void KameleonWrapper::progressBar(int current, int end) {
	float progress = static_cast<float>(current) / static_cast<float>(end-1);
	int iprogress = static_cast<int>(progress*100.0f);
	int barWidth = 70;
	if (iprogress != _lastiProgress) {
		int pos = barWidth * progress;
		int eqWidth = pos+1;
		int spWidth = barWidth - pos + 2;
		std::cout   << "[" << std::setfill('=') << std::setw(eqWidth)
		<< ">" << std::setfill(' ') << std::setw(spWidth)
		<< "] " << iprogress << " %  \r" << std::flush;
	}
	_lastiProgress = iprogress;
}

glm::vec3 KameleonWrapper::classifyFieldline(FieldlineEnd fEnd, FieldlineEnd bEnd) {
	glm::vec3 color;
	if (		(fEnd == FieldlineEnd::NORTH || fEnd == FieldlineEnd::SOUTH)
			&& 	(bEnd == FieldlineEnd::NORTH || bEnd == FieldlineEnd::SOUTH)) {
		// closed
		color = glm::vec3(1.0, 0.0, 0.0);
	} else if ((fEnd == FieldlineEnd::OUT && bEnd == FieldlineEnd::NORTH)
			|| (bEnd == FieldlineEnd::OUT && fEnd == FieldlineEnd::NORTH)) {
		// north
		color = glm::vec3(1.0, 1.0, 0.0);
	} else if ((fEnd == FieldlineEnd::OUT && bEnd == FieldlineEnd::SOUTH)
			|| (bEnd == FieldlineEnd::OUT && fEnd == FieldlineEnd::SOUTH)) {
		// south
		color = glm::vec3(0.0, 1.0, 0.0);
	} else if (fEnd == FieldlineEnd::OUT && bEnd == FieldlineEnd::OUT) {
		// solar wind
		color = glm::vec3(0.0, 0.0, 1.0);
	}
	return color;
}

} // namespace openspace
