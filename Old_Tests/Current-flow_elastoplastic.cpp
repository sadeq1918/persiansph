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

double H,HS,U,RhoF,g,D,CsW,dx,d,Muw;
double Z0,U0,Z,Us;
size_t check =0;
double DampF,DampS,DampTime;
bool Onset;

void UserInFlowCon(SPH::Domain & domi)
{
	if (domi.Time>=DampTime)
	{
		Us = U0/7.0*pow((d/(H+HS-Z0)),(1.0/7.0));
		Z = 2.5*d/30.0*(1.0-exp(-Us*2.5*d/(27.0*Muw/RhoF))) + (Muw/RhoF)/(9.0*Us);

		if (check == 0)
		{
			#pragma omp parallel for schedule (static) num_threads(domi.Nproc)
			for (size_t i=0; i<domi.Particles.Size(); i++)
			{
				if (domi.Particles[i]->ID == 10) 
				{
					domi.Particles[i]->v  = Us/0.4*log((H+HS-Z0)/Z) , 0.0 , 0.0;
					domi.Particles[i]->vb = Us/0.4*log((H+HS-Z0)/Z) , 0.0 , 0.0;

				}
			}
			std::cout<<"check point = 0"<<std::endl;
			check = 1;
		}

		int temp;
		for (int q1=0;  q1<2                ; q1++)
		for (int q2=0;  q2<domi.CellNo[1]   ; q2++)
		for (int q3=0;  q3<domi.CellNo[2]   ; q3++)
			if (domi.HOC[q1][q2][q3]!=-1)
			{
				temp = domi.HOC[q1][q2][q3];
				while (temp != -1)
				{
					if (domi.Particles[temp]->IsFree)
					{
						if (domi.Particles[temp]->ID==1)
						{
							domi.Particles[temp]->dDensity = 0.0;
							domi.Particles[temp]->Density  = RhoF*pow((1+7.0*g*(H+HS-domi.Particles[temp]->x(1))/(CsW*CsW)),(1.0/7.0));
							domi.Particles[temp]->Densityb = RhoF*pow((1+7.0*g*(H+HS-domi.Particles[temp]->x(1))/(CsW*CsW)),(1.0/7.0));
		    					domi.Particles[temp]->Pressure = SPH::EOS(domi.Particles[temp]->PresEq, domi.Particles[temp]->Cs, domi.Particles[temp]->P0,
														domi.Particles[temp]->Density,domi.Particles[temp]->RefDensity);
							domi.Particles[temp]->Mu	= domi.Particles[temp]->MuRef; 	
						}
						if (domi.Particles[temp]->x(1)> Z0)
						{
							domi.Particles[temp]->v  = Us/0.4*log((domi.Particles[temp]->x(1)-Z0)/Z) , 0.0 , 0.0;
							domi.Particles[temp]->vb = Us/0.4*log((domi.Particles[temp]->x(1)-Z0)/Z) , 0.0 , 0.0;
						}
						if (domi.Particles[temp]->x(1)<= Z0  && domi.Particles[temp]->ID==1)
						{
							domi.Particles[temp]->v(1)  = 0.0;
							domi.Particles[temp]->vb(1) = 0.0;
							domi.Particles[temp]->v(2)  = 0.0;
							domi.Particles[temp]->vb(2) = 0.0;
							if (domi.Particles[temp]->v(0)<0 )
							{
								domi.Particles[temp]->v(0) = 0.0;
								domi.Particles[temp]->vb(0) = 0.0;
							}
						}
						if (domi.Particles[temp]->x(1)<= Z0  && domi.Particles[temp]->ID==2)
						{
	//						domi.Particles[temp]->v(1)  = 0.0;
							domi.Particles[temp]->vb(1) = 0.0;
							domi.Particles[temp]->v(2)  = 0.0;
							domi.Particles[temp]->vb(2) = 0.0;
							if (domi.Particles[temp]->v(0)<0 )
							{
	//							domi.Particles[temp]->v(0) = 0.0;
								domi.Particles[temp]->vb(0) = 0.0;
							}
						}
					}
					temp = domi.Particles[temp]->LL;
				}
			}
	}
}

void UserAllFlowCon(Vec3_t & position, Vec3_t & Vel, double & Den, SPH::Boundary & bdry)
{
	Us = U0/7.0*pow((d/(H+HS-Z0)),(1.0/7.0));
	Z = 2.5*d/30.0*(1.0-exp(-Us*2.5*d/(27.0*Muw/RhoF))) + (Muw/RhoF)/(9.0*Us);

	if (position(1)>Z0)
		Vel = Us/0.4*log((position(1)-Z0)/Z) , 0.0 , 0.0;
}

void UserDamping(SPH::Domain & domi)
{
	if (domi.Time<DampTime)
	{
		#pragma omp parallel for schedule (static) num_threads(domi.Nproc)
		for (size_t i=0; i<domi.Particles.Size(); i++)
		{
			if (domi.Particles[i]->IsFree && domi.Particles[i]->Material == 1) domi.Particles[i]->a -= DampF * domi.Particles[i]->v;
			if (domi.Particles[i]->IsFree && domi.Particles[i]->Material == 3) domi.Particles[i]->a -= DampS * domi.Particles[i]->v;
		}
	}
	else
	{
		if (check == 1)
		{
			#pragma omp parallel for schedule (static) num_threads(domi.Nproc)
			for (size_t i=0; i<domi.Particles.Size(); i++)
			{
				if (domi.Particles[i]->IsFree)
				{ 
					domi.Particles[i]->v = 0.0;;
					domi.Particles[i]->vb = 0.0;;
				}
			}
			std::cout<<"check point = 1"<<std::endl;
			check = 2;
		}

		int temp;
		for (int q1=0;  q1<2                ; q1++)
		for (int q2=0;  q2<domi.CellNo[1]   ; q2++)
		for (int q3=0;  q3<domi.CellNo[2]   ; q3++)
			if (domi.HOC[q1][q2][q3]!=-1)
			{
				temp = domi.HOC[q1][q2][q3];
				while (temp != -1)
				{
					if (domi.Particles[temp]->IsFree)
					{
						if (domi.Particles[temp]->ID==1)
							domi.Particles[temp]->dDensity  = 0.0;
						if (domi.Particles[temp]->x(1)>Z0)
						{
							domi.Particles[temp]->a = 0.0 , 0.0 , 0.0;
						}
						if (domi.Particles[temp]->x(1)<=Z0 && domi.Particles[temp]->ID==1)
						{
							domi.Particles[temp]->a(1) = 0.0;
							domi.Particles[temp]->a(2) = 0.0;
						}
						if (domi.Particles[temp]->x(1)<=Z0 && domi.Particles[temp]->ID==2)
						{
//							domi.Particles[temp]->a(0) = 0.0;
							domi.Particles[temp]->a(1) = 0.0;
							domi.Particles[temp]->a(2) = 0.0;
						}
					}
					temp = domi.Particles[temp]->LL;
				}
			}

		#pragma omp parallel for schedule (static) num_threads(domi.Nproc)
		for (size_t i=0; i<domi.Particles.Size(); i++)
		{
			if (domi.Particles[i]->ID == 2 && (domi.Particles[i]->x(0) > 0.90 || domi.Particles[i]->x(0) <0.2) && domi.Particles[i]->ZWab<0.6)
			{ 
				domi.Particles[i]->a =  0.0;
				domi.Particles[i]->v =  0.0;
				domi.Particles[i]->vb =  0.0;
			}
		}
	}

	if (domi.Time>=DampTime && domi.Time<2.0 && !Onset)
	{
		#pragma omp parallel for schedule (static) num_threads(domi.Nproc)
		for (size_t i=0; i<domi.Particles.Size(); i++)
		{
			if (domi.Particles[i]->ID == 2)
			{ 
				domi.Particles[i]->a =  0.0;
				domi.Particles[i]->v =  0.0;
				domi.Particles[i]->vb =  0.0;
			}
		}
	}


}

void NewUserOutput(SPH::Particle * Particles, double & Prop1, double & Prop2,  double & Prop3)
{
	Prop1 = Particles->Mu-Particles->MuRef;
	Prop2 = Particles->n;
	Prop3 = Particles->k;
}


using std::cout;
using std::endl;
using std::ifstream;

int main(int argc, char **argv) try
{
	if (argc<2) throw new Fatal("This program must be called with one argument: the name of the data input file without the '.inp' suffix.\nExample:\t %s filekey\n",argv[0]);
	String filekey  (argv[1]);
	String filename (filekey+".inp");
	ifstream infile(filename.CStr());
	double IPhi;
	double IC;
	double IOnset;
	infile >> IPhi;		infile.ignore(200,'\n');
	infile >> IC;		infile.ignore(200,'\n');
	infile >> IOnset;	infile.ignore(200,'\n');

        SPH::Domain	dom;

        dom.Dimension	= 2;
        dom.Nproc	= 24;
    	dom.VisEq	= 0;
    	dom.KernelType	= 4;
	dom.SWIType	= 0;
    	dom.Scheme	= 0;
    	dom.Gravity	= 0.0 , -9.81 , 0.0 ;
	g		= norm(dom.Gravity);

    	double x,y,h,t,t1,t2,L;
    	dx	= 0.005;
    	h	= dx*1.1;
	D	= 0.1;
	HS	= 1.0*D;
	H	= 3.5*D;
	x	= 3.0*D;
	y	= HS + 0.5*D;
	L	= 10.0*D;

	U	= 0.4;
	Z0	= HS + 0.5*dx;
	U0	= U;
	RhoF	= 1000.0;
	CsW	= 10.0*sqrt(2.0*g*(H+HS))*2.0;
	Muw	= 0.8e-3;
        t1	= (0.25*h/(CsW));

//	dom.BC.allv		= U,0.0,0.0;
//	dom.AllCon		= & UserAllFlowCon;
        dom.GeneralBefore	= & UserInFlowCon;
        dom.GeneralAfter	= & UserDamping;
        dom.BC.Periodic[0]	= true;


    	dom.AddBoxLength(1 ,Vec3_t ( 0.0 , -3.0*dx , 0.0 ), L + dx/10.0 , H + HS + 6.0*dx + dx/10.0 ,  0 , dx/2.0 ,RhoF, h, 1 , 0 , false, false );

    	double yb,xb,R,mass,no;;

    	for (size_t a=0; a<dom.Particles.Size(); a++)
    	{
    		xb=dom.Particles[a]->x(0);
    		yb=dom.Particles[a]->x(1);

    		dom.Particles[a]->Cs		= CsW;
    		dom.Particles[a]->Alpha		= 0.05;
    		dom.Particles[a]->Beta		= 0.05;
    		dom.Particles[a]->PresEq	= 1;
    		dom.Particles[a]->Mu		= Muw;
    		dom.Particles[a]->MuRef		= Muw;
    		dom.Particles[a]->Material	= 1;
//    		dom.Particles[a]->Shepard	= true;
    		dom.Particles[a]->LES		= true;
    		dom.Particles[a]->P0		= CsW*CsW*RhoF*0.0001;
    		dom.Particles[a]->Density	= RhoF*pow((1+7.0*g*(H+HS-yb)/(CsW*CsW)),(1.0/7.0));
    		dom.Particles[a]->Densityb	= RhoF*pow((1+7.0*g*(H+HS-yb)/(CsW*CsW)),(1.0/7.0));


    		if (yb<0.0)
    		{
    			dom.Particles[a]->ID		= 4;
    			dom.Particles[a]->IsFree	= false;
    			dom.Particles[a]->NoSlip	= true;
    		}
    		if (yb>(H+HS))
    		{
    			dom.Particles[a]->ID		= 10;
    			dom.Particles[a]->IsFree	= false;
    			dom.Particles[a]->NoSlip	= true;
    		}
     		if ((pow((xb-x),2)+pow((yb-y),2))<pow((D/2.0+dx/2.0),2.0))
	     		dom.Particles[a]->ID		= 20;
    	}

   	dom.DelParticles(20);
    	mass = RhoF*dx*dx;
	R = D/2.0-dx/2.0;
    	for (size_t j=0;j<5;j++)
    	{
    		if (j>0) R -= dx;
    		no = ceil(2*M_PI*R/dx);
    		for (size_t i=0; i<no; i++)
    		{
    			xb = x + R*cos(2*M_PI/no*i);
    			yb = y + R*sin(2*M_PI/no*i);
    			dom.AddSingleParticle(4,Vec3_t ( xb ,  yb , 0.0 ), mass , RhoF , h , true);
        		dom.Particles[dom.Particles.Size()-1]->Cs	= CsW;
        		dom.Particles[dom.Particles.Size()-1]->Alpha	= 0.05;
        		dom.Particles[dom.Particles.Size()-1]->PresEq	= 1;
        		dom.Particles[dom.Particles.Size()-1]->Mu	= Muw;
        		dom.Particles[dom.Particles.Size()-1]->MuRef	= Muw;
        		dom.Particles[dom.Particles.Size()-1]->Material	= 1;
        		dom.Particles[dom.Particles.Size()-1]->NoSlip	= true;
			dom.Particles[dom.Particles.Size()-1]->FPMassC  = 1.2;
       		}
    	}

	double Nu,E,K,G,CsS,RhoS,c,Phi,Psi,n,hs,dxs;

	hs	= h/2.0;
	dxs	= dx/2.0;

	Nu	= 0.3;
	E	= 10.0e6;
	K	= E/(3.0*(1.0-2.0*Nu));
	G	= E/(2.0*(1.0+Nu));
	n	= 0.5;
	RhoS	= 2500.0*(1.0-n)+n*RhoF;
	CsS	= sqrt(K/(RhoS-RhoF));
	CsS	= sqrt(K/RhoS);
	c	= IC;
	Phi	= IPhi;
	Psi	= 0.0;
	d	= 0.00048;
        t2	= (0.25*hs/(CsS));
	Onset	= bool (IOnset);

        std::cout<<"CsS  = "<<CsS<<std::endl;
        std::cout<<"CsW  = "<<CsW<<std::endl;
        std::cout<<"RhoS = "<<RhoS<<std::endl;
        std::cout<<"Phi  = "<<Phi<<std::endl;
        std::cout<<"C    = "<<c<<std::endl;
        std::cout<<"Onset= "<<Onset<<std::endl;

	hs	= h;
	dxs	= dx;
	dom.AddBoxLength(2 ,Vec3_t ( 0.0 , -3.0*dxs , 0.0 ), 0.1 + dxs/10.0 , HS + 3.0*dxs + dxs/10.0 ,  0 , dxs/2.0 ,RhoS, hs, 1 , 0 , false, false );
	dom.AddBoxLength(2 ,Vec3_t ( 0.1 , -3.0*dxs , 0.0 ), 0.4 + dxs/10.0 , HS/2.0 + 3.0*dxs + dxs/10.0 ,  0 , dxs/2.0 ,RhoS, hs, 1 , 0 , false, false );
	hs	= h/2.0;
	dxs	= dx/2.0;
	dom.AddBoxLength(2 ,Vec3_t ( 0.1 , HS/2.0 , 0.0 ), 0.4 + dxs/10.0 , HS/2.0 + dxs/10.0 ,  0 , dxs/2.0 ,RhoS, hs, 1 , 0 , false, false );
	hs	= h;
	dxs	= dx;
	dom.AddBoxLength(2 ,Vec3_t ( 0.5 , -3.0*dxs , 0.0 ), L - 0.5   + dxs/10.0 , HS + 3.0*dxs + dxs/10.0 ,  0 , dxs/2.0 ,RhoS, hs, 1 , 0 , false, false );
	hs	= h/2.0;
	dxs	= dx/2.0;

   	mass = RhoS*dxs*dxs;
	R = D/2.0-dxs/2.0;
    	for (size_t j=0;j<5;j++)
    	{
    		if (j>0) R -= dxs;
    		no = ceil(2*M_PI*R/dxs);
    		for (size_t i=0; i<no; i++)
   		{
    			xb = x + R*cos(2*M_PI/no*i);
    			yb = y + R*sin(2*M_PI/no*i);
    			dom.AddSingleParticle(3,Vec3_t ( xb ,  yb , 0.0 ), mass , RhoS , hs , true);
       		}
    	}


	for (size_t a=0; a<dom.Particles.Size(); a++)
	{
		if (dom.Particles[a]->ID==2 || dom.Particles[a]->ID==3)
		{
			dom.Particles[a]->Material	= 3;
			dom.Particles[a]->Alpha		= 0.2;
			dom.Particles[a]->Beta		= 0.2;
			if (c>0.0)
			{
				dom.Particles[a]->TI		= 0.5;
				dom.Particles[a]->TIn		= 2.55;
			}
			dom.Particles[a]->TIInitDist	= dom.Particles[a]->h/1.1;
			dom.Particles[a]->d		= d;
//	    		dom.Particles[a]->Shepard	= true;
//			dom.Particles[a]->VarPorosity	= true;
			dom.Particles[a]->SeepageType	= 1;	
//			dom.Particles[a]->n		= n;
			dom.Particles[a]->n0		= n;
//			dom.Particles[a]->k		= k;
			dom.Particles[a]->RhoF		= RhoF;
			dom.Particles[a]->Cs		= CsS;
			dom.Particles[a]->G		= G;
			dom.Particles[a]->K		= K;
			dom.Particles[a]->Fail		= 3;	//non-associated flow rule 
			dom.Particles[a]->c		= c;
			dom.Particles[a]->phi		= Phi/180.0*M_PI;
			dom.Particles[a]->psi		= Psi/180.0*M_PI;

			xb=dom.Particles[a]->x(0);
			yb=dom.Particles[a]->x(1);

			if (dom.Particles[a]->ID == 3)
			{
				dom.Particles[a]->k 		= 1.0e30;
				dom.Particles[a]->SeepageType	= 0;	
			}
			if (yb<0.0)
			{
				dom.Particles[a]->ID	= 3;
				dom.Particles[a]->IsFree= false;
				dom.Particles[a]->NoSlip= true;
			}
			if (xb>=(x-D) && xb<=(x+D) && yb>=(D-0.1*D*sin(M_PI/2.0/D*(xb-x+D))) && dom.Particles[a]->IsFree && !Onset)
			{
		     		dom.Particles[a]->ID		= 20;
			}

		}
	}

	if (!Onset) dom.DelParticles(20);
    	DampF	= 0.02*CsW/h;
  	DampS	= 0.02*sqrt(E/(RhoS*hs*hs));
    	DampTime= 0.3;

        t	= std::min(t1,t2);
        std::cout<<"t1 = "<<t1<<std::endl;
        std::cout<<"t2 = "<<t2<<std::endl;
        std::cout<<"t  = "<<t<<std::endl;

	dom.OutputName[0]	= "MuLES";
	dom.OutputName[1]	= "Porosity";
	dom.OutputName[2]	= "Permeability";
        dom.UserOutput		= & NewUserOutput;

   	dom.Solve(/*tf*/700.0,/*dt*/t,/*dtOut*/0.1,"test",100000);
        return 0;
}
MECHSYS_CATCH
