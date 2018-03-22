/*
 * conversorSR.hpp
 *
 *  Created on: 22 de fev de 2018
 *      Author: Ruan
 */

#ifndef SRC_CONVERSORSR_HPP_
#define SRC_CONVERSORSR_HPP_
/*
 * Universidade Federal do Vale do São Francisco
 * Exercício 2 de Computação Gráfica - 14/12/2017
 * Professor Jorge Luis Cavalcanti
 * Aluno:	Ruan de Medeiros Bahia
 *
 * Conversor entre Sistemas de Referência:
 * - SRU - Sistema de Referência do Universo
 * - SRD - Sistema de Referência do Dispositivo
 */
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#define X_MIN 0
#define X_MAX 1
#define Y_MIN 2
#define Y_MAX 3

using namespace std;
int getXSRU(int *SRU, int *SRD,int xSRD);
int getXSRD(int *SRU, int *SRD,int xSRU);
int getYSRU(int *SRU, int *SRD,int ySRD);
int getYSRD(int *SRU, int *SRD,int ySRU);

double getXSRU(double *SRU, double *SRD,double xSRD);
double getXSRD(double *SRU, double *SRD,double xSRU);
double getYSRU(double *SRU, int *SRD,double ySRD);
double getYSRD(double *SRU, double *SRD,double ySRU);

int getXSRU(int *SRU, int *SRD,int xSRD){
	//  (xSRD - Xdmin)/(xSRU - Xumin) = (Xdmax - Xdmin)/(Xumax - Xumin)
	int xSRU = ((xSRD-SRD[X_MIN])*(SRU[X_MAX]-SRU[X_MIN])/(SRD[X_MAX]-SRD[X_MIN])) + SRU[X_MIN];
	return xSRU;
}

int getXSRD(int *SRU, int *SRD,int xSRU){
	//  (xSRD - Xdmin)/(xSRU - Xumin) = (Xdmax - Xdmin)/(Xumax - Xumin)
	int xSRD = ((xSRU-SRU[X_MIN])*(SRD[X_MAX]-SRD[X_MIN])/(SRU[X_MAX]-SRU[X_MIN])) + SRD[X_MIN];
	return xSRD;
}

int getYSRU(int *SRU, int *SRD,int ySRD){
	//  (ySRD - Ydmax)/(ySRU - Yumin) = (Xdmin - Xdmax)/(Yumax - Yumin)
	int ySRU = ((ySRD-SRD[Y_MAX])*(SRU[Y_MAX]-SRU[Y_MIN])/(SRD[Y_MIN]-SRD[Y_MAX])) + SRU[Y_MIN];
	return ySRU;
}
int getYSRD(int *SRU, int *SRD,int ySRU){
	//  (ySRD - Ydmax)/(ySRU - Yumin) = (Xdmin - Xdmax)/(Yumax - Yumin)
	int ySRD = ((ySRU-SRU[Y_MIN])*(SRD[Y_MIN]-SRD[Y_MAX])/(SRU[Y_MAX]-SRU[Y_MIN])) + SRD[Y_MAX];
	return ySRD;
}

double getXSRU(double *SRU, double *SRD,double xSRD){
	//  (xSRD - Xdmin)/(xSRU - Xumin) = (Xdmax - Xdmin)/(Xumax - Xumin)
	double xSRU = ((xSRD-SRD[X_MIN])*(SRU[X_MAX]-SRU[X_MIN])/(SRD[X_MAX]-SRD[X_MIN])) + SRU[X_MIN];
	return xSRU;
}
double getXSRD(double *SRU, double *SRD,double xSRU){
	//  (xSRD - Xdmin)/(xSRU - Xumin) = (Xdmax - Xdmin)/(Xumax - Xumin)
	double xSRD = ((xSRU-SRU[X_MIN])*(SRD[X_MAX]-SRD[X_MIN])/(SRU[X_MAX]-SRU[X_MIN])) + SRD[X_MIN];
	return xSRD;
}
double getYSRU(double *SRU, double *SRD,double ySRD){
	//  (ySRD - Ydmax)/(ySRU - Yumin) = (Xdmin - Xdmax)/(Yumax - Yumin)
	double ySRU = ((ySRD-SRD[Y_MAX])*(SRU[Y_MAX]-SRU[Y_MIN])/(SRD[Y_MIN]-SRD[Y_MAX])) + SRU[Y_MIN];
	return ySRU;
}
double getYSRD(double *SRU, double *SRD,double ySRU){
	//  (ySRD - Ydmax)/(ySRU - Yumin) = (Xdmin - Xdmax)/(Yumax - Yumin)
	double ySRD = ((ySRU-SRU[Y_MIN])*(SRD[Y_MIN]-SRD[Y_MAX])/(SRU[Y_MAX]-SRU[Y_MIN])) + SRD[Y_MAX];
	return ySRD;
}

#endif /* SRC_CONVERSORSR_HPP_ */
