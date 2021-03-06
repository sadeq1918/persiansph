/***********************************************************************************
* PersianSPH - A C++ library to simulate Mechanical Systems (solids, fluids        * 
*             and soils) using Smoothed Particle Hydrodynamics method              *   
* Copyright (C) 2013 Maziar Gholami Korzani and Sergio Galindo-Torres              *
*                                                                                  *
* This file is part of PersianSPH                                                  *
*                                                                                  *
* This is free software; you can redistribute it and/or modify it under the        *
* terms of the GNU General Public License as published by the Free Software        *
* Foundation; either version 3 of the License, or (at your option) any later       *
* version.                                                                         *
*                                                                                  *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY  *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A  *
* PARTICULAR PURPOSE. See the GNU General Public License for more details.         *
*                                                                                  *
* You should have received a copy of the GNU General Public License along with     *
* PersianSPH; if not, see <http://www.gnu.org/licenses/>                           *
************************************************************************************/

#include "Domain.h"
#include "Interaction.h"


using std::cout;
using std::endl;
using std::ifstream;

int check = 0;
double DampS,DampTime,Cs,u;
double t = 0.5;
double tim = 0.0;
void UserDamping(SPH::Domain & domi)
{
	if (domi.Time<DampTime)
	{
		#pragma omp parallel for schedule (static) num_threads(domi.Nproc)
		for (size_t i=0; i<domi.Particles.Size(); i++)
		{
			if (domi.Particles[i]->IsFree && domi.Particles[i]->Material == 3) domi.Particles[i]->a -= DampS * domi.Particles[i]->v;
		}
	}
}

void UserInFlowCon(Vec3_t & position, Vec3_t & Vel, double & Den, SPH::Boundary & bdry)
{
	Vel = u,0.0,0.0;
	Den = 998.21*pow((1+7.0*9.81*(0.35-position(1))/(Cs*Cs)),(1.0/7.0));
}
void NewUserOutput(SPH::Particle * Particles, double & Prop1, double & Prop2,  double & Prop3)
{
	Prop1 = Particles->ZWab;
	Prop2 = Particles->n;
	Prop3 = Particles->S;
}

int main(int argc, char **argv) try
{
        SPH::Domain	dom;

        dom.Dimension	= 2;
        dom.Nproc	= 24;
    	dom.VisEq	= 0;
    	dom.KernelType	= 0;
	dom.SWIType	= 3;
    	dom.Scheme	= 0;
    	dom.Gravity	= 0.0 , -9.81 , 0.0 ;

	u		= 0.5175;
	dom.BC.InOutFlow= 1;
	dom.BC.inv	= u,0.0,0.0;
	dom.BC.inDensity= 998.21;
	dom.InCon	= & UserInFlowCon;
	dom.GeneralAfter= & UserDamping;

	double xb,yb,h,dx,T;

	dx		= 0.02;
	h		= dx*1.3;
	dom.InitialDist	= dx;

	double rhoF,Mu,CsF,Tf;
	rhoF		= 998.23;
	Mu		= 1.0e-3;
	CsF		= 10.0*4.0;
	Tf		= (0.25*h/CsF);

	Cs = CsF;


	dom.AddBoxLength(1 ,Vec3_t ( -2.0 - 4.0*h , -3.0*dx , 0.0 ), 4.0 + 4.0*h + dx/10.0 , 1.0 + 3.0*dx + dx/10.0  ,  0 , dx/2.0 ,rhoF, h,1 , 0 , false,false);
	dom.AddBoxLength(1 ,Vec3_t (  2.0 -dx/6.0 , -10.0*dx, 0.0 ), 3.0*dx + dx/10.0      , 10.0*dx + dx/10.0       ,  0 , dx/2.0 ,rhoF, h,1 , 0 , false,false);

	for (size_t a=0; a<dom.Particles.Size(); a++)
	{
		dom.Particles[a]->PresEq	= 1;
		dom.Particles[a]->Alpha		= 0.05;
		dom.Particles[a]->Mu		= Mu;
		dom.Particles[a]->MuRef		= Mu;
		dom.Particles[a]->Material	= 1;
		dom.Particles[a]->Cs		= CsF;
		dom.Particles[a]->Shepard	= true;
		dom.Particles[a]->ShepardStep	= 20;
		xb=dom.Particles[a]->x(0);
		yb=dom.Particles[a]->x(1);
		if (yb<0.0)
		{
			dom.Particles[a]->Mass  = dom.Particles[a]->Mass*1.1;
			dom.Particles[a]->ID	= 2;
			dom.Particles[a]->NoSlip= false;
			dom.Particles[a]->IsFree= false;
		}
		if (yb>0.1 && xb<=-2.0)
		{
			dom.Particles[a]->ID	= 2;
			dom.Particles[a]->NoSlip= false;
			dom.Particles[a]->IsFree= false;
		}
		if (xb>-2.0 && dom.Particles[a]->ID == 1)
			dom.Particles[a]->ID	= 5;

//		dom.Particles[a]->Density  = rhoF*pow((1+7.0*9.81*(0.3-dom.Particles[a]->x(1))/(CsF*CsF)),(1.0/7.0));
	}

	double K,G,Nu,E,rhoS,CsS,Phi,c,Psi,Ts,n,de,Rho;

	rhoS		= 1490.0;
   	Rho		= 578.0;
	Phi		= 37.0;
	Psi		= 0.0;
	de		= 0.0255;
	n		= 0.41;
	c		= 0.5e3;
	E		= 25.0e6;
	Nu		= 0.3;
	K		= E/(3.0*(1.0-2.0*Nu));
	G		= E/(2.0*(1.0+Nu));
	CsS		= sqrt(K/rhoS);
	Ts		= (0.2*h/CsS);

	std::cout<<"CsS = "<<CsS<<std::endl;
	std::cout<<"C   = "<<c<<std::endl;
	std::cout<<"Phi = "<<Phi<<std::endl;
	std::cout<<"Psi = "<<Psi<<std::endl;

  	DampS	= 0.02*sqrt(E/(rhoS*h*h));
    	DampTime= 0.5;


	dom.AddBoxLength(3 ,Vec3_t ( -2.0 , -3.0*dx , 0.0 ), 4.0 + dx/10.0 , 1.0 + 3.0*dx + dx/10.0  ,  0 , dx/2.0 ,rhoS, h,1 , 0 , false,false);

	for (size_t a=0; a<dom.Particles.Size(); a++)
	{
		if (dom.Particles[a]->ID==3)
		{
			dom.Particles[a]->Material	= 3;
			dom.Particles[a]->Alpha		= 0.1;
			dom.Particles[a]->Beta		= 0.1;
			dom.Particles[a]->TI		= 0.5;
			dom.Particles[a]->TIn		= 2.55;
			dom.Particles[a]->TIInitDist	= dx;
			dom.Particles[a]->d		= de;
			dom.Particles[a]->VarPorosity	= true;
			dom.Particles[a]->SeepageType	= 3;	
			dom.Particles[a]->n0		= n;
			dom.Particles[a]->RhoF		= Rho;
			dom.Particles[a]->Cs		= CsS;
			dom.Particles[a]->G		= G;
			dom.Particles[a]->K		= K;
			dom.Particles[a]->Fail		= 3;	//non-associated flow rule 
			dom.Particles[a]->c		= c;
			dom.Particles[a]->phi		= Phi/180.0*M_PI;
			dom.Particles[a]->psi		= Psi/180.0*M_PI;
	    		dom.Particles[a]->IsSat		= false;
			xb=dom.Particles[a]->x(0);
			yb=dom.Particles[a]->x(1);
			if (yb<0.0)
			{
				dom.Particles[a]->ID	= 4;
				dom.Particles[a]->IsFree= false;
				dom.Particles[a]->NoSlip= true;
			}
			if (yb>(-(1.0/1.5)*(xb-1.6)) && dom.Particles[a]->ID == 3)
				dom.Particles[a]->ID	= 5;
			if (yb>( (1.0/1.5)*(xb+1.6)) && dom.Particles[a]->ID == 3)
				dom.Particles[a]->ID	= 5;
		}
	}
	dom.DelParticles(5);

	T	= std::min(Tf,Ts);

	std::cout<<"Tf = "<<Tf<<std::endl;
	std::cout<<"Ts = "<<Ts<<std::endl;
	std::cout<<"T  = "<<T<<std::endl;

	dom.OutputName[0]	= "ZWab";
	dom.OutputName[1]	= "Porosity";
	dom.OutputName[2]	= "S_lift";
        dom.UserOutput		= & NewUserOutput;

	dom.Solve(/*tf*/50000.0,/*dt*/T,/*dtOut*/0.1,"test06",2000);
	return 0;
}
MECHSYS_CATCH
