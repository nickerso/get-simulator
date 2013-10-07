/*
 * kinsol.hpp
 *
 *  Created on: Sep 13, 2012
 *      Author: dnic019
 */

#ifndef KINSOL_HPP_
#define KINSOL_HPP_

class GeneralModel;

/**
 * Utility to wrap KINSOL for use in get.
 */
int solveOneVariable(GeneralModel* model, double minimumValue, double maximumValue);

#endif /* KINSOL_HPP_ */
