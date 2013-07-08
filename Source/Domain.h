/************************************************************************
 * MechSys - Open Library for Mechanical Systems                        *
 * Copyright (C) 2013 Maziar Gholami Korzani and Sergio Galindo Torres  *
 *                                                                      *
 * This program is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * any later version.                                                   *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the         *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program. If not, see <http://www.gnu.org/licenses/>  *
 ************************************************************************/

#ifndef MECHSYS_SPH_DOMAIN_H
#define MECHSYS_SPH_DOMAIN_H

// Std Lib
#include <stdio.h>			/// for NULL
#include <algorithm>		/// for min,max


#include <Source/Interaction.h>

// HDF File Output
#include <hdf5.h>
#include <hdf5_hl.h>

#include <mechsys/util/stopwatch.h>

namespace SPH {



class Domain
{
public:

    // Constructor
    Domain();  ///< Constructor with a vector containing the number of divisions per length, and Xmin Xmax defining the limits of the rectangular domain to be plotted

    // Destructor
    ~Domain ();

    // Methods
    void AddBox              (int tag, Vec3_t const & x, size_t nx, size_t ny, size_t nz,
    		                  double R, double Mass, double Density, double h, bool Fixed);                                    ///< Add a box of particles (should specify radius of particles)
    void AddBoxLength        (int tag, Vec3_t const &V, double Lx, double Ly, double Lz, size_t nx, size_t ny, size_t nz,
    		                  double Mass, double Density, double h, bool Fixed);                                              ///< Add a box of particles with length (calculate radius of particles)
    void AddRandomBox        (int tag, Vec3_t const &V, double Lx, double Ly, double Lz, size_t nx, size_t ny, size_t nz,
    		                  double Mass, double Density, double h, size_t RandomSeed=100);                                   ///< Add box of random positioned particles (calculate radius of particles)
    void StartAcceleration   (Vec3_t const & a = Vec3_t(0.0,0.0,0.0));                                                         ///< Add a fixed acceleration
    void ComputeAcceleration (double dt);                                                                                      ///< Compute the acceleration due to the other particles
    void Move                (double dt);                                                                                      ///< Compute the acceleration due to the other particles
    void ResetInteractions();                                                                                                  ///< Reset the interaction array
    void ResetContacts();                                                                                                      ///< Reset the possible interactions
    void Domain::DelParticles(Array<int> const & Tags);																		   ///< Delete particles by tags
    void Solve               (double tf, double dt, double dtOut, char const * TheFileKey, size_t Nproc);                      ///< The solving function
    void WriteXDMF           (char const * FileKey);                                                                           ///< Save a XDMF file for visualization
    void Save                (char const * FileKey);                                                         				   ///< Save the domain form a file
    void Load                (char const * FileKey);                                                         				   ///< Load the domain form a file

    // Data
    Vec3_t                  Gravity;        ///< Gravity acceleration
    Array <Particle*>       Particles;      ///< Array of particles
    Array <Interaction*>    Interactions;   ///< Array of interactions
    Array <Interaction*>    PInteractions;  ///< Array of possible interactions
    double                  Time;           ///< The simulation Time
    size_t                  idx_out;        ///< Index for output purposes
    double 					Dimension;      ///< Dimension of the problem
    double 					Alpha;
    double					Beta;
    double					MaxVel;
    double					AutoSaveInt;		///< Automatic save interval
};

/// A structure for the multi-thread data
struct MtData
{
    size_t                       ProcRank; ///< Rank of the thread
    size_t                         N_Proc; ///< Total number of threads
    SPH::Domain *                     Dom; ///< Pointer to the SPH domain
    Vec3_t                            Acc; ///< Prefixed acceleration for the particles
    double						   Deltat; ///< Prefixed dt for the interactions
};

void * GlobalStartAcceleration(void * Data)
{
    SPH::MtData & dat = (*static_cast<SPH::MtData *>(Data));
    Array<SPH::Particle * > * P = &dat.Dom->Particles;
	size_t Ni = P->Size()/dat.N_Proc;
    size_t In = dat.ProcRank*Ni;
    size_t Fn;
    dat.ProcRank == dat.N_Proc-1 ? Fn = P->Size() : Fn = (dat.ProcRank+1)*Ni;
	for (size_t i=In;i<Fn;i++)
	{
		(*P)[i]->a = dat.Acc;
		(*P)[i]->dDensity = 0.0;
	}
    return NULL;
}

void * GlobalComputeAcceleration(void * Data)
{
    SPH::MtData & dat = (*static_cast<SPH::MtData *>(Data));
    Array<SPH::Interaction * > * P = &dat.Dom->Interactions;
	size_t Ni = P->Size()/dat.N_Proc;
    size_t In = dat.ProcRank*Ni;
    size_t Fn;
    dat.ProcRank == dat.N_Proc-1 ? Fn = P->Size() : Fn = (dat.ProcRank+1)*Ni;
	for (size_t i=In;i<Fn;i++)
	{
		(*P)[i]->CalcForce(dat.Deltat);
	}
    return NULL;
}

void * GlobalMove(void * Data)
{
    SPH::MtData & dat = (*static_cast<SPH::MtData *>(Data));
    Array<SPH::Particle * > * P = &dat.Dom->Particles;
	size_t Ni = P->Size()/dat.N_Proc;
    size_t In = dat.ProcRank*Ni;
    size_t Fn;
    dat.ProcRank == dat.N_Proc-1 ? Fn = P->Size() : Fn = (dat.ProcRank+1)*Ni;
	for (size_t i=In;i<Fn;i++)
	{
		(*P)[i]->Move(dat.Deltat);
			}
    return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////// Implementation /////

// Constructor
inline Domain::Domain ()
{
    Time    = 0.0;
    Gravity = 0.0,0.0,0.0;
    idx_out = 0;

}

inline Domain::~Domain ()
{
    for (size_t i=0; i<Particles.Size();   ++i) if (Particles  [i]!=NULL) delete Particles  [i];
    for (size_t i=0; i<Interactions.Size(); ++i) if (Interactions[i]!=NULL) delete Interactions[i];
}


// Methods
inline void Domain::AddBox(int tag, Vec3_t const & V, size_t nx, size_t ny, size_t nz, double R, double Mass, double Density, double h, bool Fixed)
{
    std::cout << "\n--------------Generating particles by AddBox with defined radius-----------------------------------" << std::endl;

    for (size_t i = 0;i<nx;i++)
    for (size_t j = 0;j<ny;j++)
    for (size_t k = 0;k<nz;k++)
    {
        Vec3_t x(2*i*R,2*j*R,2*k*R);
        x +=V;
        if (Mass == 0.0)
        	Particles.Push(new Particle(tag,x,Vec3_t(0,0,0),4/3*M_PI*R*R*R*Density,Density,R,h,Fixed));
        else
        	Particles.Push(new Particle(tag,x,Vec3_t(0,0,0),Mass,Density,R,h,Fixed));
    }
    std::cout << "\n  Total No. of particles   = " << Particles.Size() << std::endl;
}

inline void Domain::AddBoxLength(int tag, Vec3_t const & V, double Lx, double Ly, double Lz, size_t nx, size_t ny, size_t nz, double Mass, double Density, double h, bool Fixed)
{
    std::cout << "\n--------------Generating particles by AddBoxLength with defined length-----------------------------" << std::endl;

	double R = (std::min(Lx/(2*nx),Ly/(2*ny))>0) ? std::min(Lx/(2*nx),Ly/(2*ny)) : std::max(Lx/(2*nx),Ly/(2*ny));
    R = (std::min(R,Lz/(2*nz))>0) ? std::min(R,Lz/(2*nz)) : std::max(R,Lz/(2*nz));

	for (size_t i=0; i<nx; i++)
    {
        for (size_t j=0; j<ny; j++)
        {
            for (size_t k=0; k<nz; k++)
            {
                double x = V(0)+i*Lx/nx;
                double y = V(1)+j*Ly/ny;
                double z = V(2)+k*Lz/nz;
                if (Mass == 0.0)
                	Particles.Push(new Particle(tag,Vec3_t(x,y,z),Vec3_t(0,0,0),4/3*M_PI*R*R*R*Density,Density,R,h,Fixed));
                else
                	Particles.Push(new Particle(tag,Vec3_t(x,y,z),Vec3_t(0,0,0),Mass,Density,R,h,Fixed));
            }
        }
    }
    std::cout << "\n  Total No. of particles   = " << Particles.Size() << std::endl;
}

inline void Domain::AddRandomBox(int tag, Vec3_t const & V, double Lx, double Ly, double Lz, size_t nx, size_t ny, size_t nz, double Mass, double Density, double h, size_t RandomSeed)
{
    Util::Stopwatch stopwatch;
    std::cout << "\n--------------Generating random packing of particles by AddRandomBox-------------------------------" << std::endl;

    double R = (std::min(Lx/(2*nx),Ly/(2*ny))>0) ? std::min(Lx/(2*nx),Ly/(2*ny)) : std::max(Lx/(2*nx),Ly/(2*ny));
    R = (std::min(R,Lz/(2*nz))>0) ? std::min(R,Lz/(2*nz)) : std::max(R,Lz/(2*nz));

	std::cout << R << std::endl;

    double qin = 0.95;
    srand(RandomSeed);
    for (size_t i=0; i<nx; i++)
    {
        for (size_t j=0; j<ny; j++)
        {
            for (size_t k=0; k<nz; k++)
            {
                double x = V(0)+(i+0.5*qin+(1-qin)*double(rand())/RAND_MAX)*Lx/nx;
                double y = V(1)+(j+0.5*qin+(1-qin)*double(rand())/RAND_MAX)*Ly/ny;
                double z = V(2)+(k+0.5*qin+(1-qin)*double(rand())/RAND_MAX)*Lz/nz;
                if (Mass == 0.0)
                	Particles.Push(new Particle(tag,Vec3_t(x,y,z),Vec3_t(0,0,0),4/3*M_PI*R*R*R*Density,Density,R,h,false));
                else
                	Particles.Push(new Particle(tag,Vec3_t(x,y,z),Vec3_t(0,0,0),Mass,Density,R,h,false));
            }
        }
    }
    std::cout << "\n  Total No. of particles   = " << Particles.Size() << std::endl;
}

inline void Domain::StartAcceleration (Vec3_t const & a)
{
    for (size_t i=0; i<Particles.Size(); i++)
    {
        Particles[i]->a = a;
        Particles[i]->dDensity = 0.0;
    }
}

inline void Domain::ComputeAcceleration (double dt)
{
//    for (size_t i=0; i<PInteractions.Size(); i++) PInteractions[i]->CalcForce(dt);
	for (size_t i=0; i<Interactions.Size(); i++) Interactions[i]->CalcForce(dt);
}

inline void Domain::Move (double dt)
{
    for (size_t i=0; i<Particles.Size(); i++) Particles[i]->Move(dt);
}

inline void Domain::DelParticles (Array<int> const & Tags)
{
    Array<int> idxs; // indices to be deleted
    for (size_t i=0; i<Particles.Size(); ++i)
    {
        for (size_t j=0; j<Tags.Size(); ++j)
        {
            if (Particles[i]->ID==Tags[j]) idxs.Push(i);
        }
    }
    if (idxs.Size()<1) throw new Fatal("Domain::DelParticles: Could not find any particle to be deleted");
    Particles.DelItems (idxs);
}


inline void Domain::ResetInteractions()
{
    // delete old interactors
    for (size_t i=0; i<Interactions.Size(); ++i)
    {
        if (Interactions[i]!=NULL) delete Interactions[i];
    }

    // new interactors
    Interactions.Resize(0);
    for (size_t i=0; i<Particles.Size()-1; i++)
    {
        for (size_t j=i+1; j<Particles.Size(); j++)
        {
            // if both particles are fixed, don't create any interactor
            if (!Particles[i]->IsFree && !Particles[j]->IsFree) continue;
            else Interactions.Push(new Interaction(Particles[i],Particles[j],Dimension,Alpha,Beta,MaxVel));
        }
    }
}

inline void Domain::ResetContacts()
{
    PInteractions.Resize(0);
    for (size_t i=0; i<Interactions.Size(); i++)
    {
        if(Interactions[i]->UpdateContacts()) PInteractions.Push(Interactions[i]);
    }
}

inline void Domain::WriteXDMF (char const * FileKey)
{

	String fn(FileKey);
    fn.append(".h5");
    hid_t file_id;
    file_id = H5Fcreate(fn.CStr(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);


    float * Posvec   = new float[3*Particles.Size()];
    float * Velvec   = new float[3*Particles.Size()];
    float * Pressure = new float[  Particles.Size()];
    float * Radius   = new float[  Particles.Size()];
    int   * Tag      = new int  [  Particles.Size()];

    for (size_t i=0;i<Particles.Size();i++)
    {
        Posvec  [3*i  ] = float(Particles[i]->x(0));
        Posvec  [3*i+1] = float(Particles[i]->x(1));
        Posvec  [3*i+2] = float(Particles[i]->x(2));
        Velvec  [3*i  ] = float(Particles[i]->v(0));
        Velvec  [3*i+1] = float(Particles[i]->v(1));
        Velvec  [3*i+2] = float(Particles[i]->v(2));
        Pressure[i    ] = float(Particles[i]->Pressure);
        Radius  [i    ] = float(Particles[i]->R);
        Tag     [i    ] = int  (Particles[i]->ID);
    }

    hsize_t dims[1];
    dims[0] = 3*Particles.Size();
    String dsname;
    dsname.Printf("Position");
    H5LTmake_dataset_float(file_id,dsname.CStr(),1,dims,Posvec);
    dsname.Printf("Velocity");
    H5LTmake_dataset_float(file_id,dsname.CStr(),1,dims,Velvec);
    dims[0] = Particles.Size();
    dsname.Printf("Pressure");
    H5LTmake_dataset_float(file_id,dsname.CStr(),1,dims,Pressure);
    dsname.Printf("Radius");
    H5LTmake_dataset_float(file_id,dsname.CStr(),1,dims,Radius);
    dsname.Printf("Tag");
    H5LTmake_dataset_int(file_id,dsname.CStr(),1,dims,Tag);


    delete [] Posvec;
    delete [] Velvec;
    delete [] Pressure;
    delete [] Radius;
    delete [] Tag;


    //Closing the file
    H5Fflush(file_id,H5F_SCOPE_GLOBAL);
    H5Fclose(file_id);


    //Writing xmf file
    std::ostringstream oss;
    oss << "<?xml version=\"1.0\" ?>\n";
    oss << "<!DOCTYPE Xdmf SYSTEM \"Xdmf.dtd\" []>\n";
    oss << "<Xdmf Version=\"2.0\">\n";
    oss << " <Domain>\n";
    oss << "   <Grid Name=\"SPHCenter\" GridType=\"Uniform\">\n";
    oss << "     <Topology TopologyType=\"Polyvertex\" NumberOfElements=\"" << Particles.Size() << "\"/>\n";
    oss << "     <Geometry GeometryType=\"XYZ\">\n";
    oss << "       <DataItem Format=\"HDF\" NumberType=\"Float\" Precision=\"4\" Dimensions=\"" << Particles.Size() << " 3\" >\n";
    oss << "        " << fn.CStr() <<":/Position \n";
    oss << "       </DataItem>\n";
    oss << "     </Geometry>\n";
    oss << "     <Attribute Name=\"Velocity\" AttributeType=\"Vector\" Center=\"Node\">\n";
    oss << "       <DataItem Dimensions=\"" << Particles.Size() << " 3\" NumberType=\"Float\" Precision=\"4\" Format=\"HDF\">\n";
    oss << "        " << fn.CStr() <<":/Velocity \n";
    oss << "       </DataItem>\n";
    oss << "     </Attribute>\n";
    oss << "     <Attribute Name=\"Pressure\" AttributeType=\"Scalar\" Center=\"Node\">\n";
    oss << "       <DataItem Dimensions=\"" << Particles.Size() << "\" NumberType=\"Float\" Precision=\"4\"  Format=\"HDF\">\n";
    oss << "        " << fn.CStr() <<":/Pressure \n";
    oss << "       </DataItem>\n";
    oss << "     </Attribute>\n";
    oss << "     <Attribute Name=\"Radius\" AttributeType=\"Scalar\" Center=\"Node\">\n";
    oss << "       <DataItem Dimensions=\"" << Particles.Size() << "\" NumberType=\"Float\" Precision=\"4\"  Format=\"HDF\">\n";
    oss << "        " << fn.CStr() <<":/Radius \n";
    oss << "       </DataItem>\n";
    oss << "     </Attribute>\n";
    oss << "     <Attribute Name=\"Tag\" AttributeType=\"Scalar\" Center=\"Node\">\n";
    oss << "       <DataItem Dimensions=\"" << Particles.Size() << "\" NumberType=\"Int\" Format=\"HDF\">\n";
    oss << "        " << fn.CStr() <<":/Tag \n";
    oss << "       </DataItem>\n";
    oss << "     </Attribute>\n";
    oss << "   </Grid>\n";
    oss << " </Domain>\n";
    oss << "</Xdmf>\n";


    fn = FileKey;
    fn.append(".xmf");
    std::ofstream of(fn.CStr(), std::ios::out);
    of << oss.str();
    of.close();
}

inline void Domain::Save (char const * FileKey)
{
    // Opening the file for writing
    String fn(FileKey);
    fn.append(".hdf5");

    hid_t file_id;
    file_id = H5Fcreate(fn.CStr(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    // Storing the number of particles in the domain
    int data[1];
    data[0]=Particles.Size();
    hsize_t dims[1];
    dims[0]=1;
    H5LTmake_dataset_int(file_id,"/NP",1,dims,data);

    for (size_t i=0; i<Particles.Size(); i++)
    {
        // Creating the string and the group for each particle
        hid_t group_id;
        String par;
        par.Printf("/Particle_%08d",i);
        group_id = H5Gcreate2(file_id, par.CStr(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);


        // Storing some scalar variables
        double dat[1];
        dat[0] = Particles[i]->Mass;
        H5LTmake_dataset_double(group_id,"Mass",1,dims,dat);
        dat[0] = Particles[i]->Density;
        H5LTmake_dataset_double(group_id,"Rho",1,dims,dat);
        dat[0] = Particles[i]->hr;
        H5LTmake_dataset_double(group_id,"h",1,dims,dat);
        dat[0] = Particles[i]->R;
        H5LTmake_dataset_double(group_id,"Radius",1,dims,dat);

        int tag[1];
        tag[0] = Particles[i]->ID;
        H5LTmake_dataset_int(group_id,"Tag",1,dims,tag);

        // Change  to integer for fixity
        int IsFree[1];
        if (Particles[i]->IsFree == true) IsFree[0]=1;
        		else IsFree[0]=0;
        H5LTmake_dataset_int(group_id,"IsFree",1,dims,IsFree);

        // Storing vectorial variables
        double cd[3];
        hsize_t dd[1];
        dd[0] = 3;

        cd[0]=Particles[i]->x(0);
        cd[1]=Particles[i]->x(1);
        cd[2]=Particles[i]->x(2);
        H5LTmake_dataset_double(group_id,"x",1,dd,cd);

        cd[0]=Particles[i]->v(0);
        cd[1]=Particles[i]->v(1);
        cd[2]=Particles[i]->v(2);
        H5LTmake_dataset_double(group_id,"v",1,dd,cd);
    }

    H5Fflush(file_id,H5F_SCOPE_GLOBAL);
    H5Fclose(file_id);
}

inline void Domain::Load (char const * FileKey)
{

    // Opening the file for reading
    String fn(FileKey);
    fn.append(".hdf5");
    if (!Util::FileExists(fn)) throw new Fatal("File <%s> not found",fn.CStr());
    printf("\n%s--- Loading file %s --------------------------------------------%s\n",TERM_CLR1,fn.CStr(),TERM_RST);
    hid_t file_id;
    file_id = H5Fopen(fn.CStr(), H5F_ACC_RDONLY, H5P_DEFAULT);

    // Number of particles in the domain
    int data[1];
    H5LTread_dataset_int(file_id,"/NP",data);
    size_t NP = data[0];

    // Loading the particles
    for (size_t i=0; i<NP; i++)
    {

        // Creating the string and the group for each particle
        hid_t group_id;
        String par;
        par.Printf("/Particle_%08d",i);
        group_id = H5Gopen2(file_id, par.CStr(),H5P_DEFAULT);

        Particles.Push(new Particle(0,Vec3_t(0,0,0),Vec3_t(0,0,0),0,0,0,0,false));

        // Loading vectorial variables
        double cd[3];
        H5LTread_dataset_double(group_id,"x",cd);
        Particles[Particles.Size()-1]->x = Vec3_t(cd[0],cd[1],cd[2]);
        Particles[Particles.Size()-1]->xb = Vec3_t(cd[0],cd[1],cd[2]);			// Because of the constructor in Particle

        H5LTread_dataset_double(group_id,"v",cd);
        Particles[Particles.Size()-1]->v = Vec3_t(cd[0],cd[1],cd[2]);

        // Loading the scalar quantities of the particle
        double dat[1];
        H5LTread_dataset_double(group_id,"Mass",dat);
        Particles[Particles.Size()-1]->Mass = dat[0];

        H5LTread_dataset_double(group_id,"Rho",dat);
        Particles[Particles.Size()-1]->Density = dat[0];
        Particles[Particles.Size()-1]->RefDensity = dat[0];			// Because of the constructor in Particle
        Particles[Particles.Size()-1]->Densityb = dat[0];			// Because of the constructor in Particle

        H5LTread_dataset_double(group_id,"h",dat);
        Particles[Particles.Size()-1]->hr = dat[0];
        Particles[Particles.Size()-1]->h = dat[0];			// Because of the constructor in Particle

        H5LTread_dataset_double(group_id,"Radius",dat);
        Particles[Particles.Size()-1]->R = dat[0];

        int datint[1];
        H5LTread_dataset_int(group_id,"Tag",datint);
        Particles[Particles.Size()-1]->ID = datint[0];

        H5LTread_dataset_int(group_id,"IsFree",datint);
        if (datint[0] == 1) Particles[Particles.Size()-1]->IsFree=true;
        		else Particles[Particles.Size()-1]->IsFree=false;
    }


    H5Fclose(file_id);
    printf("\n%s--- Done --------------------------------------------%s\n",TERM_CLR2,TERM_RST);
}

inline void Domain::Solve (double tf, double dt, double dtOut, char const * TheFileKey, size_t Nproc)
{
    Util::Stopwatch stopwatch;
    std::cout << "\n--------------Solving------------------------------------------------------------------------------" << std::endl;

    idx_out = 1;
    double tout = Time;

    double sout = AutoSaveInt;

    ResetInteractions();
    ResetContacts();

	SPH::MtData MTD[Nproc];
	for (size_t i=0;i<Nproc;i++)
	{
	   MTD[i].N_Proc   = Nproc;
	   MTD[i].ProcRank = i;
	   MTD[i].Dom      = this;
	   MTD[i].Acc      = Gravity;
	   MTD[i].Deltat   = dt;
	}
	pthread_t thrs[Nproc];

    while (Time<tf)
    {

    	// Calculate the acceleration for each particle
    	for (size_t i=0;i<Nproc;i++)
    	{
    	   pthread_create(&thrs[i], NULL, GlobalStartAcceleration, &MTD[i]);
    	}
    	for (size_t i=0;i<Nproc;i++)
    	{
    	   pthread_join(thrs[i], NULL);
    	}

//    	StartAcceleration(Gravity);

    	for (size_t i=0;i<Nproc;i++)
    	{
    	   pthread_create(&thrs[i], NULL, GlobalComputeAcceleration, &MTD[i]);
    	}
    	for (size_t i=0;i<Nproc;i++)
    	{
    	   pthread_join(thrs[i], NULL);
    	}
//    	ComputeAcceleration(dt);


        // Move each particle
    	for (size_t i=0;i<Nproc;i++)
    	{
    	   pthread_create(&thrs[i], NULL, GlobalMove, &MTD[i]);
    	}
    	for (size_t i=0;i<Nproc;i++)
    	{
    	   pthread_join(thrs[i], NULL);
    	}
//    	Move(dt);


        // output
        if (Time>=tout)
        {
            if (TheFileKey!=NULL)
            {
                    String fn;
                    fn.Printf    ("%s_%04d", TheFileKey, idx_out);
                    WriteXDMF    (fn.CStr());
                    std::cout << "\n" << "Output No. " << idx_out << " at " << Time << " has been generated" << std::endl;
            }
            idx_out++;
            tout += dtOut;
        }

        // Auto Save
       if (AutoSaveInt>0)
       {
        if (Time>=sout)
       {
           if (TheFileKey!=NULL)
           {
                   String fn;
                   fn.Printf    ("Auto Save", TheFileKey);
                   Save   		(fn.CStr());
                   std::cout << "\n" << "Auto Save at " << Time << " has been generated" << std::endl;
           }
       sout += AutoSaveInt;
       }
       }
//       std::cout << "Finish step" << std::endl;

       Time += dt;

        //ResetContacts();

    }
}

}; // namespace SPH

#endif // MECHSYS_SPH_DOMAIN_H
