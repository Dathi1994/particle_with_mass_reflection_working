static char help[] = "This is VTK output program written by Trung Le (lebao002@umn.edu)";

#include <vector>
#include "petscda.h"
#include "petscts.h"
#include "petscpc.h"
#include "petscsnes.h"
#include <stdio.h>
#include <stdlib.h>

#define NEWMETRIC

#ifdef TECIO
	#include "TECIO.h"
#endif

PetscInt  ti, block_number, Flux_in;
int       binary_input=0;
int       xyz_input=0;
PetscInt  tis, tie, tsteps=1;
PetscReal angle;
int       nv_once=0;
int       onlyV=0;
int       k_average=0;
int       j_average=0;
int       i_average=0;
int       ik_average=0;
int       ikc_average=0;	// conditional spatial averaging in ik directions (channel flow)
int       reynolds=0;	// 1: contravariant reynolds stress

int       i_begin, i_end;
int       j_begin, j_end;
int       k_begin, k_end;

int       pcr=0;
int       avg=0, rans=0, rans_output=0, levelset=0;
int       vc = 1;
int       vtkOutput = 0;

int       cs=0;
int       i_periodic=0;
int       j_periodic=0;
int       k_periodic=0;
int       kk_periodic=0;
int       averaging_option=0;
int       pi=-1, pk=-1;
int       shear=0;

char      prefix[256];
int 	  QCR = 0;
//int l, m, n;
/* Symmetric matrix A -> eigenvectors in columns of V, corresponding eigenvalues in d. */
void eigen_decomposition(double A[3][3], double V[3][3], double d[3]);


typedef struct {
	PetscReal t, f;
} FlowWave;

typedef struct {
	PassiveScalar u, v, w, p;
} PassiveField;

typedef struct {
	PetscScalar u, v, w;
} Field;

typedef struct {
	PetscScalar x, y, z;
} Cmpnts;

typedef struct {
	PetscScalar x, y;
} Cmpnts2;

typedef struct {
	PassiveScalar csi[3], eta[3], zet[3], aj;
} Metrics;

typedef struct {
	Vec	Ubcs; // An ugly hack, waste of memory
} BCS;


typedef struct {
	PetscInt	nbnumber;
	PetscInt	n_v, n_elmt;	// number of vertices and number of elements
	PetscInt	*nv1, *nv2, *nv3;	// Node index of each triangle
	PetscReal	*nf_x, *nf_y, *nf_z;	// Normal direction
	PetscReal	*x_bp, *y_bp, *z_bp;	// Coordinates of IBM surface nodes
	PetscReal	*x_bp0, *y_bp0, *z_bp0;
	PetscReal	*x_bp_o, *y_bp_o, *z_bp_o;
/*	PetscReal	x_bp_in[101][3270], y_bp_in[101][3270], z_bp_in[101][3270]; */
	Cmpnts	*u, *uold;
} IBMNodes;

typedef struct {
	// Additional global param for particle tracing
	PetscReal   *xpini, *ypini, *zpini;
	PetscReal   *xploc, *yploc, *zploc;
	PetscReal   *xpite, *ypite, *zpite;
	PetscReal   *pressure_pnew;
	PetscReal   *presuure_xpite, *presuure_ypite, *presuure_zpite;
	PetscReal   *uploc, *vploc, *wploc;
	PetscReal   *presuure_ploc_x, *presuure_ploc_y, *presuure_ploc_z;
	PetscReal   *pressure_ploc;
	PetscReal   dxmin, dymin, dzmin;
	PetscInt    *ip, *jp, *kp;

	PetscReal  *dpdcsi,*dpdeta,*dpdzet;
	PetscReal  *dpdx,*dpdy,*dpdz,*delp;
	PetscReal  *Fpx, *Fpy, *Fpz, *Fp, *Fb;
	//Vec ParVel;
	//Vec ParLoc;
} PT;

typedef struct {
	PetscInt	IM, JM, KM; // dimensions of grid
	DA da;	/* Data structure for scalars (include the grid geometry
						informaion, to obtain the grid information,
						use DAGetCoordinates) */
	DA fda, fda2;	// Data Structure for vectors
	//DA cda;
	DALocalInfo info;

	Vec	Cent;	// Coordinates of cell centers
	Vec Csi, Eta, Zet, Aj;
	Vec ICsi, IEta, IZet, IAj;
	Vec JCsi, JEta, JZet, JAj;
	Vec KCsi, KEta, KZet, KAj;
	Vec Ucont;	// Contravariant velocity components
	Vec Ucat;	// Cartesian velocity components
	Vec	Ucat_o;
	Vec P;
	Vec	Phi;
	Vec	GridSpace;
	Vec	Nvert;
	Vec	Nvert_o;
	BCS	Bcs;

	PetscInt  *nvert;//ody property

	PetscReal ren;	// Reynolds number
	PetscReal dt; 	// time step
	PetscReal st;	// Strouhal number

	PetscReal r[101], tin[101], uinr[101][1001];

	Vec	      lUcont, lUcat, lP, lPhi;
	Vec	      lCsi, lEta, lZet, lAj;
	Vec	      lICsi, lIEta, lIZet, lIAj;
	Vec	      lJCsi, lJEta, lJZet, lJAj;
	Vec	      lKCsi, lKEta, lKZet, lKAj;
	Vec	      lGridSpace;
	Vec	      lNvert, lNvert_o;
	Vec	      lCent;
	Vec       Pressure_Gradient;
	Vec       Ucat_sum;		// u, v, w
	Vec       Ucat_cross_sum;		// uv, vw, wu
	Vec       Ucat_square_sum;	// u^2, v^2, w^2

	PetscInt  _this;

	FlowWave  *inflow, *kinematics;
	PetscInt  number_flowwave, number_kinematics;

	PetscInt  num_of_particle;
	Vec       Dist;
	PetscReal *dcutoff;
	PetscReal *xpini, *ypini, *zpini;
	PetscReal *xploc, *yploc, *zploc;
	PetscReal *xpite, *ypite, *zpite;
	PetscReal *pressure_pnew;
	PetscReal *presuure_xpite, *presuure_ypite, *presuure_zpite;
	PetscReal *uploc, *vploc, *wploc;
	PetscReal *presuure_ploc_x, *presuure_ploc_y, *presuure_ploc_z;
	PetscReal *presuure_ploc;

	PetscReal  *dpdcsi,*dpdeta,*dpdzet;
	PetscReal  *dpdx,*dpdy,*dpdz,*delp;
	PetscReal  *Fpx, *Fpy, *Fpz, *Fp, *Fb;

	PetscReal dxmin, dymin, dzmin;
	PetscInt  *ip, *jp, *kp;
	PetscInt  speedscale;
	PetscInt  starting_k;
	PetscReal  dp, rho, rho_p, vis, g, Vf;
	


	// This is my modification
	//
} UserCtx;

PetscErrorCode ReadCoordinates(UserCtx *user);
PetscErrorCode QCriteria(UserCtx *user);
PetscErrorCode Velocity_Magnitude(UserCtx *user);
PetscErrorCode Lambda2(UserCtx *user);
PetscErrorCode FormMetrics(UserCtx *user);

void Calc_avg_shear_stress(UserCtx *user);

PetscErrorCode VtkOutput(UserCtx *user, int only_V)
{
	PetscInt	i, j, k, bi, numbytes;
	Cmpnts		***ucat, ***coor, ***ucat_o, ***parloc, ***parvel;
	PetscReal	***p, ***nvert, ***level;
	Vec			Coor, Levelset, K_Omega;
	FILE		*f, *fblock;
	char		filen[80], filen2[80];
	PetscInt	rank;
	size_t		offset = 0;

	printf("Writing VTK Output\n");

	PetscPrintf(PETSC_COMM_WORLD,"after vtk output");

	if (cs) {
		printf("Option \"cs\" not implemented in VTK output!\n");
		return -1;
	}

	MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

	if (!rank) {

		if (block_number > 0) {
			sprintf(filen2, "Result%5.5d.vtm", ti);
			fblock = fopen(filen2, "w");
			// VTK header
			PetscFPrintf(PETSC_COMM_WORLD, fblock, "<VTKFile type=\"vtkMultiBlockDataSet\" version=\"1.0\" byte_order=\"LittleEndian\" header_type=\"UInt32\">\n");
			PetscFPrintf(PETSC_COMM_WORLD, fblock, "  <vtkMultiBlockDataSet>\n");
		}

		for (bi=0; bi<block_number; bi++) {
			DA			da   = user[bi].da;
			DA			fda  = user[bi].fda;
			DALocalInfo	info = user[bi].info;

			// grid count
			PetscInt	xs = info.xs, xe = info.xs + info.xm;
			PetscInt	ys = info.ys, ye = info.ys + info.ym;
			PetscInt	zs = info.zs, ze = info.zs + info.zm;
			PetscInt	mx = info.mx, my = info.my, mz = info.mz;

			i_begin = 1, i_end = mx-1;	// cross section in tecplot
			j_begin = 1, j_end = my-1;
			k_begin = 1, k_end = mz-1;

			// use options if specified
			PetscOptionsGetInt(PETSC_NULL, "-i_begin", &i_begin, PETSC_NULL);
			PetscOptionsGetInt(PETSC_NULL, "-i_end", &i_end, PETSC_NULL);
			PetscOptionsGetInt(PETSC_NULL, "-j_begin", &j_begin, PETSC_NULL);
			PetscOptionsGetInt(PETSC_NULL, "-j_end", &j_end, PETSC_NULL);
			PetscOptionsGetInt(PETSC_NULL, "-k_begin", &k_begin, PETSC_NULL);
			PetscOptionsGetInt(PETSC_NULL, "-k_end", &k_end, PETSC_NULL);

			xs = i_begin - 1, xe = i_end+1;
			ys = j_begin - 1, ye = j_end+1;
			zs = k_begin - 1, ze = k_end+1;

			DAGetCoordinates(da, &Coor);
			DAVecGetArray(fda, Coor, &coor);
			if (only_V != 2) DAVecGetArray(da,  user[bi].Nvert, &nvert);
			if (!only_V) DAVecGetArray(da,  user[bi].P, &p);
			if (!vc) DAVecGetArray(fda, user[bi].Ucat_o, &ucat_o);
			//if (QCR==3) DAVecGetArray(fda,  user[bi].Pressure_Gradient, &pressure_gradient);
			else {
				DAVecGetArray(fda, user[bi].Ucat, &ucat);
				/*
				DAVecGetArray(fda, user[bi].ParLoc, &parloc);
				DAVecGetArray(fda, user[bi].ParVel, &parvel);
				*/
			}


			// Check if petsc_real is a double variable. If this is not the case and you want to make the code work
			// then you need to change the type="Float64" output below to the correct size
			if (PETSC_REAL != PETSC_DOUBLE) {
				printf("PETSC_REAL is not equal to PETSC_DOUBLE which conflicts with the vtk Output\n");
				break;
			}

			sprintf(filen, "Result%5.5d_%2.2d.vts", ti, bi);
			f = fopen(filen, "w");

			// write entry to multiblock file
			if (block_number > 0) {
				PetscFPrintf(PETSC_COMM_WORLD, fblock, "    <DataSet index=\"%d\" file=\"%s\">\n", bi, filen);
				PetscFPrintf(PETSC_COMM_WORLD, fblock, "    </DataSet>\n");
			}

			// VTK header
			PetscFPrintf(PETSC_COMM_WORLD, f, "<VTKFile type=\"StructuredGrid\" version=\"1.0\" byte_order=\"LittleEndian\" header_type=\"UInt32\">\n");
			PetscFPrintf(PETSC_COMM_WORLD, f, "  <StructuredGrid WholeExtent=\"%d %d %d %d %d %d\">\n", xs, xe-2, ys, ye-2, zs, ze-2);
			PetscFPrintf(PETSC_COMM_WORLD, f, "    <Piece Extent=\"%d %d %d %d %d %d\">\n", xs, xe-2, ys, ye-2, zs, ze-2);

			if (!vc) {
				// point data header
				PetscFPrintf(PETSC_COMM_WORLD, f, "      <PointData>\n");
				// u ca
				PetscFPrintf(PETSC_COMM_WORLD, f, "        <DataArray type=\"Float64\" Name=\"Ucat_o\" NumberOfComponents=\"3\" format=\"appended\" offset=\"%d\"/>\n", offset);
				offset += (ze-1-zs)*(ye-1-ys)*(xe-1-xs)*sizeof(double)*3+sizeof(int);
				// point data footer
				PetscFPrintf(PETSC_COMM_WORLD, f, "      </PointData>\n");
			}

			// cell data header
			PetscFPrintf(PETSC_COMM_WORLD, f, "      <CellData>\n");
			// pressure
			if (!only_V) {
				PetscFPrintf(PETSC_COMM_WORLD, f, "        <DataArray type=\"Float64\" Name=\"P\" format=\"appended\" offset=\"%d\"/>\n", offset);
				offset += (ze-2-zs)*(ye-2-ys)*(xe-2-xs)*sizeof(double)+sizeof(int);
			}
			// ucat
			if (vc) {
				PetscFPrintf(PETSC_COMM_WORLD, f, "        <DataArray type=\"Float64\" Name=\"Ucat\" NumberOfComponents=\"3\" format=\"appended\" offset=\"%d\"/>\n", offset);
				offset += (ze-2-zs)*(ye-2-ys)*(xe-2-xs)*sizeof(double)*3+sizeof(int);

				/*
				PetscFPrintf(PETSC_COMM_WORLD, f, "        <DataArray type=\"Float64\" Name=\"ParLoc\" NumberOfComponents=\"3\" format=\"appended\" offset=\"%d\"/>\n", offset);
				offset += (ze-2-zs)*(ye-2-ys)*(xe-2-xs)*sizeof(double)*3+sizeof(int);

				PetscFPrintf(PETSC_COMM_WORLD, f, "        <DataArray type=\"Float64\" Name=\"ParVel\" NumberOfComponents=\"3\" format=\"appended\" offset=\"%d\"/>\n", offset);
				offset += (ze-2-zs)*(ye-2-ys)*(xe-2-xs)*sizeof(double)*3+sizeof(int);
				*/
			}
			// pressure gradient
			if (QCR==3) {
				PetscFPrintf(PETSC_COMM_WORLD, f, "        <DataArray type=\"Float64\" Name=\"Pressure_Gradient\" NumberOfComponents=\"3\" format=\"appended\" offset=\"%d\"/>\n", offset);
				offset += (ze-2-zs)*(ye-2-ys)*(xe-2-xs)*sizeof(double)*3+sizeof(int);

				/*
				PetscFPrintf(PETSC_COMM_WORLD, f, "        <DataArray type=\"Float64\" Name=\"ParLoc\" NumberOfComponents=\"3\" format=\"appended\" offset=\"%d\"/>\n", offset);
				offset += (ze-2-zs)*(ye-2-ys)*(xe-2-xs)*sizeof(double)*3+sizeof(int);

				PetscFPrintf(PETSC_COMM_WORLD, f, "        <DataArray type=\"Float64\" Name=\"ParVel\" NumberOfComponents=\"3\" format=\"appended\" offset=\"%d\"/>\n", offset);
				offset += (ze-2-zs)*(ye-2-ys)*(xe-2-xs)*sizeof(double)*3+sizeof(int);
				*/
			}
			// nvert
			if (only_V != 2) {
				PetscFPrintf(PETSC_COMM_WORLD, f, "        <DataArray type=\"Float64\" Name=\"Nvert\" format=\"appended\" offset=\"%d\"/>\n", offset);
				offset += (ze-2-zs)*(ye-2-ys)*(xe-2-xs)*sizeof(double)+sizeof(int);
			}
			// rans (k, omega, nut)
			if (!onlyV && rans) {
				PetscFPrintf(PETSC_COMM_WORLD, f, "        <DataArray type=\"Float64\" Name=\"k\" format=\"appended\" offset=\"%d\"/>\n", offset);
				offset += (ze-2-zs)*(ye-2-ys)*(xe-2-xs)*sizeof(double)+sizeof(int);
				PetscFPrintf(PETSC_COMM_WORLD, f, "        <DataArray type=\"Float64\" Name=\"omega\" format=\"appended\" offset=\"%d\"/>\n", offset);
				offset += (ze-2-zs)*(ye-2-ys)*(xe-2-xs)*sizeof(double)+sizeof(int);
				PetscFPrintf(PETSC_COMM_WORLD, f, "        <DataArray type=\"Float64\" Name=\"nut\" format=\"appended\" offset=\"%d\"/>\n", offset);
				offset += (ze-2-zs)*(ye-2-ys)*(xe-2-xs)*sizeof(double)+sizeof(int);
			}
			// levelset
			if (levelset) {
				PetscFPrintf(PETSC_COMM_WORLD, f, "        <DataArray type=\"Float64\" Name=\"Levelset\" format=\"appended\" offset=\"%d\"/>\n", offset);
				offset += (ze-2-zs)*(ye-2-ys)*(xe-2-xs)*sizeof(double)+sizeof(int);
			}
			// point data footer
			PetscFPrintf(PETSC_COMM_WORLD, f, "      </CellData>\n");

			// coordinate saving
			// header
			PetscFPrintf(PETSC_COMM_WORLD, f, "      <Points>\n");
			PetscFPrintf(PETSC_COMM_WORLD, f, "        <DataArray type=\"Float64\" Name=\"Points\" NumberOfComponents=\"3\" format=\"appended\" offset=\"%d\"/>\n", offset);
			offset += (ze-1-zs)*(ye-1-ys)*(xe-1-xs)*sizeof(double)*3+sizeof(int);
			// footer
			PetscFPrintf(PETSC_COMM_WORLD, f, "      </Points>\n");

			// piece footer
			PetscFPrintf(PETSC_COMM_WORLD, f, "    </Piece>\n");
			PetscFPrintf(PETSC_COMM_WORLD, f, "  </StructuredGrid>\n");

			// data is following next
			PetscFPrintf(PETSC_COMM_WORLD, f, "  <AppendedData encoding=\"raw\">\n_");

			// ucat_o
			if (!vc) {
				numbytes = sizeof(double)*(xe-1-xs)*(ye-1-ys)*(ze-1-zs)*3;
				fwrite(&numbytes, sizeof(int), 1, f);
				for (k=zs; k<ze-1; k++) {
					for (j=ys; j<ye-1; j++) {
						for (i=xs; i<xe-1; i++) {
							double value[3];
							value[0] = ucat_o[k][j][i].x;
							value[1] = ucat_o[k][j][i].y;
							value[2] = ucat_o[k][j][i].z;
							fwrite(value, sizeof(double), 3, f);
						}
					}
				}
			}

			// pressure
			if (!only_V) {
				numbytes = sizeof(double)*(xe-2-xs)*(ye-2-ys)*(ze-2-zs);
				fwrite(&numbytes, sizeof(int), 1, f);
				for (k=zs; k<ze-2; k++) {
					for (j=ys; j<ye-2; j++) {
						for (i=xs; i<xe-2; i++) {
							double value = p[k+1][j+1][i+1];
							fwrite(&value, sizeof(value), 1, f);
						}
					}
				}
			}

			// ucat
			if (vc) {
				numbytes = sizeof(double)*(xe-2-xs)*(ye-2-ys)*(ze-2-zs)*3;
				fwrite(&numbytes, sizeof(int), 1, f);
				for (k=zs; k<ze-2; k++) {
					for (j=ys; j<ye-2; j++) {
						for (i=xs; i<xe-2; i++) {
							double value[3];
							value[0] = ucat[k+1][j+1][i+1].x;
							value[1] = ucat[k+1][j+1][i+1].y;
							value[2] = ucat[k+1][j+1][i+1].z;
							fwrite(value, sizeof(double), 3, f);
						}
					}
				}
		/*		
		    // pressure gradient
		    if (QCR==3) {
				numbytes = sizeof(double)*(xe-2-xs)*(ye-2-ys)*(ze-2-zs)*3;
				fwrite(&numbytes, sizeof(int), 1, f);
				for (k=zs; k<ze-2; k++) {
					for (j=ys; j<ye-2; j++) {
						for (i=xs; i<xe-2; i++) {
							double value[3];
							value[0] = pressure_gradient[k+1][j+1][i+1].x;
							value[1] = pressure_gradient[k+1][j+1][i+1].y;
							value[2] = pressure_gradient[k+1][j+1][i+1].z;
							fwrite(value, sizeof(double), 3, f);
						}
					}
				}
			}*/


				/*
				numbytes = sizeof(double)*(xe-2-xs)*(ye-2-ys)*(ze-2-zs)*3;
				fwrite(&numbytes, sizeof(int), 1, f);
				for (k=zs; k<ze-2; k++) {
					for (j=ys; j<ye-2; j++) {
						for (i=xs; i<xe-2; i++) {
							double value[3];
							value[0] = parloc[k+1][j+1][i+1].x;
							value[1] = parloc[k+1][j+1][i+1].y;
							value[2] = parloc[k+1][j+1][i+1].z;
							fwrite(value, sizeof(double), 3, f);
						}
					}
				}

				numbytes = sizeof(double)*(xe-2-xs)*(ye-2-ys)*(ze-2-zs)*3;
				fwrite(&numbytes, sizeof(int), 1, f);
				for (k=zs; k<ze-2; k++) {
					for (j=ys; j<ye-2; j++) {
						for (i=xs; i<xe-2; i++) {
							double value[3];
							value[0] = parvel[k+1][j+1][i+1].x;
							value[1] = parvel[k+1][j+1][i+1].y;
							value[2] = parvel[k+1][j+1][i+1].z;
							fwrite(value, sizeof(double), 3, f);
						}
					}
				}
				*/
			}

			// nvert
			if (only_V != 2) {
				numbytes = sizeof(double)*(xe-2-xs)*(ye-2-ys)*(ze-2-zs);
				fwrite(&numbytes, sizeof(int), 1, f);
				for (k=zs; k<ze-2; k++) {
					for (j=ys; j<ye-2; j++) {
						for (i=xs; i<xe-2; i++) {
							double value = nvert[k+1][j+1][i+1];
							fwrite(&value, sizeof(value), 1, f);
						}
					}
				}
			}

			// rans (k, omega, nut)
			if (!onlyV && rans) {
				Cmpnts2 ***komega;
				DACreateGlobalVector(user[bi].fda2, &K_Omega);
				PetscViewer	viewer;
				sprintf(filen, "kfield%05d_%1d.dat", ti, user->_this);
				PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
				VecLoadIntoVector(viewer, K_Omega);
				PetscViewerDestroy(viewer);
				DAVecGetArray(user[bi].fda2, K_Omega, &komega);

				numbytes = sizeof(double)*(xe-2-xs)*(ye-2-ys)*(ze-2-zs);
				fwrite(&numbytes, sizeof(int), 1, f);
				for (k=zs; k<ze-2; k++) {
					for (j=ys; j<ye-2; j++) {
						for (i=xs; i<xe-2; i++) {
							double value = komega[k+1][j+1][i+1].x;
							fwrite(&value, sizeof(value), 1, f);
						}
					}
				}

				numbytes = sizeof(double)*(xe-2-xs)*(ye-2-ys)*(ze-2-zs);
				fwrite(&numbytes, sizeof(int), 1, f);
				for (k=zs; k<ze-2; k++) {
					for (j=ys; j<ye-2; j++) {
						for (i=xs; i<xe-2; i++) {
							double value = komega[k+1][j+1][i+1].y;
							fwrite(&value, sizeof(value), 1, f);
						}
					}
				}

				numbytes = sizeof(double)*(xe-2-xs)*(ye-2-ys)*(ze-2-zs);
				fwrite(&numbytes, sizeof(int), 1, f);
				for (k=zs; k<ze-2; k++) {
					for (j=ys; j<ye-2; j++) {
						for (i=xs; i<xe-2; i++) {
							double value = komega[k+1][j+1][i+1].x/(komega[k+1][j+1][i+1].y+1e-20);;
							fwrite(&value, sizeof(value), 1, f);
						}
					}
				}

				DAVecRestoreArray(user[bi].fda2, K_Omega, &komega);
				VecDestroy(K_Omega);
			}

			// levelset
			if (levelset) {
				// get levelset data
				DACreateGlobalVector(da, &Levelset);
				PetscViewer viewer;
				sprintf(filen, "lfield%05d_%1d.dat", ti, user->_this);
				PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
				VecLoadIntoVector(viewer, Levelset);
				PetscViewerDestroy(viewer);
				DAVecGetArray(da, Levelset, &level);

				// write data
				numbytes = sizeof(double)*(xe-2-xs)*(ye-2-ys)*(ze-2-zs);
				fwrite(&numbytes, sizeof(int), 1, f);
				for (k=zs; k<ze-2; k++) {
					for (j=ys; j<ye-2; j++) {
						for (i=xs; i<xe-2; i++) {
							double value = level[k+1][j+1][i+1];
							fwrite(&value, sizeof(value), 1, f);
						}
					}
				}

				// cleanup
				DAVecRestoreArray(da, Levelset, &level);
				VecDestroy(Levelset);
			}

			// coordinates
			numbytes = sizeof(double)*(xe-1-xs)*(ye-1-ys)*(ze-1-zs)*3;
			fwrite(&numbytes, sizeof(int), 1, f);
			for (k=zs; k<ze-1; k++) {
				for (j=ys; j<ye-1; j++) {
					for (i=xs; i<xe-1; i++) {
						double value[3];
						value[0] = coor[k][j][i].x;
						value[1] = coor[k][j][i].y;
						value[2] = coor[k][j][i].z;
						fwrite(value, sizeof(double), 3, f);
					}
				}
			}

			// end of data
			PetscFPrintf(PETSC_COMM_WORLD, f, "  </AppendedData>\n");

			DAVecRestoreArray(fda, Coor, &coor);
			if (only_V != 2) DAVecRestoreArray(da , user[bi].Nvert, &nvert);
			if (!only_V) DAVecRestoreArray(da , user[bi].P, &p);
			if (vc) DAVecRestoreArray(fda, user[bi].Ucat, &ucat);
			//if (QCR==3) DAVecRestoreArray(fda, user[bi].Pressure_Gradient, &pressure_gradient);
			else DAVecRestoreArray(fda, user[bi].Ucat_o, &ucat_o);

			// VTK footer
			PetscFPrintf(PETSC_COMM_WORLD, f, "</VTKFile>\n");

			// close structured grid file
			fclose(f);

		} // end of block loop

		if (block_number > 0) {
			// VTK header
			PetscFPrintf(PETSC_COMM_WORLD, fblock, "  </vtkMultiBlockDataSet>\n");
			PetscFPrintf(PETSC_COMM_WORLD, fblock, "</VTKFile>\n");

			// close multiblock file
			fclose(fblock);
		}

	} // end of if (!rank)

	return(0);

}

int file_exist(char *str)
{
	int r=0;

	/*if (!my_rank)*/ {
		FILE *fp=fopen(str, "r");
		if (!fp) {
			r=0;
			printf("\nFILE !!! %s does not exist !!!\n", str);
		}
		else {
			fclose(fp);
			r=1;
		}
	}
	MPI_Bcast(&r, 1, MPI_INT, 0, PETSC_COMM_WORLD);
	return r;
};

void Calculate_Covariant_metrics(double g[3][3], double G[3][3])
{
	/*
		| csi.x  csi.y csi.z |-1		| x.csi  x.eta x.zet |
		| eta.x eta.y eta.z |	 =	| y.csi   y.eta  y.zet |
		| zet.x zet.y zet.z |		| z.csi  z.eta z.zet |
	*/

	const double a11=g[0][0], a12=g[0][1], a13=g[0][2];
	const double a21=g[1][0], a22=g[1][1], a23=g[1][2];
	const double a31=g[2][0], a32=g[2][1], a33=g[2][2];

	double det= a11*(a33*a22-a32*a23)-a21*(a33*a12-a32*a13)+a31*(a23*a12-a22*a13);

	G[0][0] = (a33*a22-a32*a23)/det,	G[0][1] = - (a33*a12-a32*a13)/det, 	G[0][2] = (a23*a12-a22*a13)/det;
	G[1][0] = -(a33*a21-a31*a23)/det, G[1][1] = (a33*a11-a31*a13)/det,	G[1][2] = - (a23*a11-a21*a13)/det;
	G[2][0] = (a32*a21-a31*a22)/det,	G[2][1] = - (a32*a11-a31*a12)/det,	G[2][2] = (a22*a11-a21*a12)/det;
};

void Calculate_normal(Cmpnts csi, Cmpnts eta, Cmpnts zet, double ni[3], double nj[3], double nk[3])
{
	double g[3][3];
	double G[3][3];

	g[0][0]=csi.x, g[0][1]=csi.y, g[0][2]=csi.z;
	g[1][0]=eta.x, g[1][1]=eta.y, g[1][2]=eta.z;
	g[2][0]=zet.x, g[2][1]=zet.y, g[2][2]=zet.z;

	Calculate_Covariant_metrics(g, G);
	double xcsi=G[0][0], ycsi=G[1][0], zcsi=G[2][0];
	double xeta=G[0][1], yeta=G[1][1], zeta=G[2][1];
	double xzet=G[0][2], yzet=G[1][2], zzet=G[2][2];

	double nx_i = xcsi, ny_i = ycsi, nz_i = zcsi;
	double nx_j = xeta, ny_j = yeta, nz_j = zeta;
	double nx_k = xzet, ny_k = yzet, nz_k = zzet;

	double sum_i=sqrt(nx_i*nx_i+ny_i*ny_i+nz_i*nz_i);
	double sum_j=sqrt(nx_j*nx_j+ny_j*ny_j+nz_j*nz_j);
	double sum_k=sqrt(nx_k*nx_k+ny_k*ny_k+nz_k*nz_k);

	nx_i /= sum_i, ny_i /= sum_i, nz_i /= sum_i;
	nx_j /= sum_j, ny_j /= sum_j, nz_j /= sum_j;
	nx_k /= sum_k, ny_k /= sum_k, nz_k /= sum_k;

	ni[0] = nx_i, ni[1] = ny_i, ni[2] = nz_i;
	nj[0] = nx_j, nj[1] = ny_j, nj[2] = nz_j;
	nk[0] = nx_k, nk[1] = ny_k, nk[2] = nz_k;
}

double Contravariant_Reynolds_stress(double uu, double uv, double uw, double vv, double vw, double ww,
	double csi0, double csi1, double csi2, double eta0, double eta1, double eta2)
{

	double A = uu*csi0*eta0 + vv*csi1*eta1 + ww*csi2*eta2 + uv * (csi0*eta1+csi1*eta0)	+ uw * (csi0*eta2+csi2*eta0) + vw * (csi1*eta2+csi2*eta1);
	double B = sqrt(csi0*csi0+csi1*csi1+csi2*csi2)*sqrt(eta0*eta0+eta1*eta1+eta2*eta2);

	return A/B;
}

void Compute_du_center (int i, int j, int k,  int mx, int my, int mz, Cmpnts ***ucat, PetscReal ***nvert,
				double *dudc, double *dvdc, double *dwdc,
				double *dude, double *dvde, double *dwde,
				double *dudz, double *dvdz, double *dwdz)
{
	if ((nvert[k][j][i+1])> 0.1) {
		*dudc = ( ucat[k][j][i].x - ucat[k][j][i-1].x );
		*dvdc = ( ucat[k][j][i].y - ucat[k][j][i-1].y );
		*dwdc = ( ucat[k][j][i].z - ucat[k][j][i-1].z );
	}
	else if ((nvert[k][j][i-1])> 0.1) {
		*dudc = ( ucat[k][j][i+1].x - ucat[k][j][i].x );
		*dvdc = ( ucat[k][j][i+1].y - ucat[k][j][i].y );
		*dwdc = ( ucat[k][j][i+1].z - ucat[k][j][i].z );
	}
	else {
		if (i_periodic && i==1) {
			*dudc = ( ucat[k][j][i+1].x - ucat[k][j][mx-2].x ) * 0.5;
			*dvdc = ( ucat[k][j][i+1].y - ucat[k][j][mx-2].y ) * 0.5;
			*dwdc = ( ucat[k][j][i+1].z - ucat[k][j][mx-2].z ) * 0.5;
		}
		else if (i_periodic && i==mx-2) {
			*dudc = ( ucat[k][j][1].x - ucat[k][j][i-1].x ) * 0.5;
			*dvdc = ( ucat[k][j][1].y - ucat[k][j][i-1].y ) * 0.5;
			*dwdc = ( ucat[k][j][1].z - ucat[k][j][i-1].z ) * 0.5;
		}
		else {
			*dudc = ( ucat[k][j][i+1].x - ucat[k][j][i-1].x ) * 0.5;
			*dvdc = ( ucat[k][j][i+1].y - ucat[k][j][i-1].y ) * 0.5;
			*dwdc = ( ucat[k][j][i+1].z - ucat[k][j][i-1].z ) * 0.5;
		}
	}

	if ((nvert[k][j+1][i])> 0.1) {
		*dude = ( ucat[k][j][i].x - ucat[k][j-1][i].x );
		*dvde = ( ucat[k][j][i].y - ucat[k][j-1][i].y );
		*dwde = ( ucat[k][j][i].z - ucat[k][j-1][i].z );
	}
	else if ((nvert[k][j-1][i])> 0.1) {
		*dude = ( ucat[k][j+1][i].x - ucat[k][j][i].x );
		*dvde = ( ucat[k][j+1][i].y - ucat[k][j][i].y );
		*dwde = ( ucat[k][j+1][i].z - ucat[k][j][i].z );
	}
	else {
		if (j_periodic && j==1) {
			*dude = ( ucat[k][j+1][i].x - ucat[k][my-2][i].x ) * 0.5;
			*dvde = ( ucat[k][j+1][i].y - ucat[k][my-2][i].y ) * 0.5;
			*dwde = ( ucat[k][j+1][i].z - ucat[k][my-2][i].z ) * 0.5;
		}
		else if (j_periodic && j==my-2) {
			*dude = ( ucat[k][1][i].x - ucat[k][j-1][i].x ) * 0.5;
			*dvde = ( ucat[k][1][i].y - ucat[k][j-1][i].y ) * 0.5;
			*dwde = ( ucat[k][1][i].z - ucat[k][j-1][i].z ) * 0.5;
		}
		else {
			*dude = ( ucat[k][j+1][i].x - ucat[k][j-1][i].x ) * 0.5;
			*dvde = ( ucat[k][j+1][i].y - ucat[k][j-1][i].y ) * 0.5;
			*dwde = ( ucat[k][j+1][i].z - ucat[k][j-1][i].z ) * 0.5;
		}
	}

	if ((nvert[k+1][j][i])> 0.1) {
		*dudz = ( ucat[k][j][i].x - ucat[k-1][j][i].x );
		*dvdz = ( ucat[k][j][i].y - ucat[k-1][j][i].y );
		*dwdz = ( ucat[k][j][i].z - ucat[k-1][j][i].z );
	}
	else if ((nvert[k-1][j][i])> 0.1) {
		*dudz = ( ucat[k+1][j][i].x - ucat[k][j][i].x );
		*dvdz = ( ucat[k+1][j][i].y - ucat[k][j][i].y );
		*dwdz = ( ucat[k+1][j][i].z - ucat[k][j][i].z );
	}
	else {
		if (k_periodic && k==1) {
			*dudz = ( ucat[k+1][j][i].x - ucat[mz-2][j][i].x ) * 0.5;
			*dvdz = ( ucat[k+1][j][i].y - ucat[mz-2][j][i].y ) * 0.5;
			*dwdz = ( ucat[k+1][j][i].z - ucat[mz-2][j][i].z ) * 0.5;
		}
		else if (k_periodic && k==mz-2) {
			*dudz = ( ucat[1][j][i].x - ucat[k-1][j][i].x ) * 0.5;
			*dvdz = ( ucat[1][j][i].y - ucat[k-1][j][i].y ) * 0.5;
			*dwdz = ( ucat[1][j][i].z - ucat[k-1][j][i].z ) * 0.5;
		}
		else {
			*dudz = ( ucat[k+1][j][i].x - ucat[k-1][j][i].x ) * 0.5;
			*dvdz = ( ucat[k+1][j][i].y - ucat[k-1][j][i].y ) * 0.5;
			*dwdz = ( ucat[k+1][j][i].z - ucat[k-1][j][i].z ) * 0.5;
		}
	}
}


void Compute_du_dxyz (	double csi0, double csi1, double csi2, double eta0, double eta1, double eta2, double zet0, double zet1, double zet2, double ajc,
					double dudc, double dvdc, double dwdc, double dude, double dvde, double dwde, double dudz, double dvdz, double dwdz,
					double *du_dx, double *dv_dx, double *dw_dx, double *du_dy, double *dv_dy, double *dw_dy, double *du_dz, double *dv_dz, double *dw_dz )
{
	*du_dx = (dudc * csi0 + dude * eta0 + dudz * zet0) * ajc;
	*du_dy = (dudc * csi1 + dude * eta1 + dudz * zet1) * ajc;
	*du_dz = (dudc * csi2 + dude * eta2 + dudz * zet2) * ajc;
	*dv_dx = (dvdc * csi0 + dvde * eta0 + dvdz * zet0) * ajc;
	*dv_dy = (dvdc * csi1 + dvde * eta1 + dvdz * zet1) * ajc;
	*dv_dz = (dvdc * csi2 + dvde * eta2 + dvdz * zet2) * ajc;
	*dw_dx = (dwdc * csi0 + dwde * eta0 + dwdz * zet0) * ajc;
	*dw_dy = (dwdc * csi1 + dwde * eta1 + dwdz * zet1) * ajc;
	*dw_dz = (dwdc * csi2 + dwde * eta2 + dwdz * zet2) * ajc;
};


void IKavg(float *x, int xs, int xe, int ys, int ye, int zs, int ze, int mx, int my, int mz)
{
	int i, j, k;

	for (j=ys; j<ye-2; j++) {
		double iksum=0;
		int count=0;
		for (i=xs; i<xe-2; i++)
		for (k=zs; k<ze-2; k++) {
			iksum += x[k * (mx-2)*(my-2) + j*(mx-2) + i];
			count++;
		}
		double ikavg = iksum/(double)count;
		for (i=xs; i<xe-2; i++)
		for (k=zs; k<ze-2; k++) x[k * (mx-2)*(my-2) + j*(mx-2) + i] = ikavg;
	}
}

/*
	pi, pk : # of grid points correcsponding to the period
	conditional averaging
*/
void IKavg_c (float *x, int xs, int xe, int ys, int ye, int zs, int ze, int mx, int my, int mz)
{
	//int i, j, k;

	if (pi<=0) pi = (xe-xs-2); // no averaging
	if (pk<=0) pk = (ze-zs-2); // no averaging

	int ni = (xe-xs-2) / pi;
	int nk = (ze-zs-2) / pk;

	std::vector< std::vector<float> > iksum (pk);

	for (int k=0; k<pk; k++) iksum[k].resize(pi);

	for (int j=ys; j<ye-2; j++) {

		for (int k=0; k<pk; k++) std::fill( iksum[k].begin(), iksum[k].end(), 0.0 );

		for (int i=xs; i<xe-2; i++)
		for (int k=zs; k<ze-2; k++) {
			iksum [ ( k - zs ) % pk ] [ ( i - xs ) % pi] += x [k * (mx-2)*(my-2) + j*(mx-2) + i];
		}

		for (int i=xs; i<xe-2; i++)
		for (int k=zs; k<ze-2; k++) x [k * (mx-2)*(my-2) + j*(mx-2) + i] = iksum [ ( k - zs ) % pk ] [ ( i - xs ) % pi ] / (ni*nk);
	}
}


void Kavg(float *x, int xs, int xe, int ys, int ye, int zs, int ze, int mx, int my, int mz)
{
	int i, j, k;

	for (j=ys; j<ye-2; j++)
	for (i=xs; i<xe-2; i++) {
		double ksum=0;
		int count=0;
		for (k=zs; k<ze-2; k++) {
			ksum += x[k * (mx-2)*(my-2) + j*(mx-2) + i];
			count++;
		}
		double kavg = ksum/(double)count;
		for (k=zs; k<ze-2; k++) x[k * (mx-2)*(my-2) + j*(mx-2) + i] = kavg;
	}

}

void Javg(float *x, int xs, int xe, int ys, int ye, int zs, int ze, int mx, int my, int mz)
{
	int i, j, k;

	for (k=zs; k<ze-2; k++)
	for (i=xs; i<xe-2; i++) {
		double jsum=0;
		int count=0;
		for (j=ys; j<ye-2; j++) {
			jsum += x[k * (mx-2)*(my-2) + j*(mx-2) + i];
			count++;
		}
		double javg = jsum/(double)count;
		for (j=ys; j<ye-2; j++) x[k * (mx-2)*(my-2) + j*(mx-2) + i] = javg;
	}

}

void Iavg(float *x, int xs, int xe, int ys, int ye, int zs, int ze, int mx, int my, int mz)
{
	int i, j, k;

	for (k=zs; k<ze-2; k++) {
		for (j=ys; j<ye-2; j++) {
			double isum=0;
			int count=0;
			for (i=xs; i<xe-2; i++) {
				isum += x[k * (mx-2)*(my-2) + j*(mx-2) + i];
				count++;
			}
			double iavg = isum/(double)count;
			for (i=xs; i<xe-2; i++) x[k * (mx-2)*(my-2) + j*(mx-2) + i] = iavg;
		}
	}
}

void Iavg(Cmpnts ***x, int xs, int xe, int ys, int ye, int zs, int ze, int mx, int my, int mz)
{
	int i, j, k;

	for (k=zs; k<ze-2; k++)
	for (j=ys; j<ye-2; j++) {
		Cmpnts isum, iavg;
		isum.x = isum.y = isum.z = 0;

		int count=0;
		for (i=xs; i<xe-2; i++) {
			 isum.x += x[k+1][j+1][i+1].x;
			 isum.y += x[k+1][j+1][i+1].y;
			 isum.z += x[k+1][j+1][i+1].z;
			 count++;
		}

		iavg.x = isum.x /(double)count;
		iavg.y = isum.y /(double)count;
		iavg.z = isum.z /(double)count;

		for (i=xs; i<xe-2; i++) x[k+1][j+1][i+1] = iavg;
	}
}

void Iavg(PetscReal ***x, int xs, int xe, int ys, int ye, int zs, int ze, int mx, int my, int mz)
{
	int i, j, k;

	for (k=zs; k<ze-2; k++)
	for (j=ys; j<ye-2; j++) {
		double isum, iavg;
		isum = 0;

		int count=0;
		for (i=xs; i<xe-2; i++) {
			 isum += x[k+1][j+1][i+1];
			 count++;
		}
		iavg = isum /(double)count;

		for (i=xs; i<xe-2; i++) x[k+1][j+1][i+1] = iavg;
	}
}

#ifdef TECIO
PetscErrorCode TECIOOut_V(UserCtx *user, int only_V)	// seokkoo
{
	PetscInt bi;

	char filen[80];
	sprintf(filen, "%sResult%05d.plt", prefix, ti);

	INTEGER4 I, Debug, VIsDouble, DIsDouble, III, IMax, JMax, KMax;
	VIsDouble = 0;
	DIsDouble = 0;
	Debug = 0;

	if (only_V)   {
		if (cs) I = TECINI100((char*)"Flow", (char*)"X Y Z UU Cs", filen, (char*)".", &Debug, &VIsDouble);
		else if (only_V==2) I = TECINI100((char*)"Flow", (char*)"X Y Z UU", filen, (char*)".", &Debug, &VIsDouble);
		else I = TECINI100((char*)"Flow", (char*)"X Y Z UU Nv", filen, (char*)".", &Debug, &VIsDouble);
	}
	else if (rans/* && rans_output*/) {
		if (levelset) I = TECINI100((char*)"Flow", (char*)"X Y Z U V W UU P Nv K Omega Nut Level", filen, (char*)".", &Debug, &VIsDouble);
		else I = TECINI100((char*)"Flow", (char*)"X Y Z U V W UU P Nv K Omega Nut ", filen, (char*)".", &Debug, &VIsDouble);
	}
	else {
		if (cs) I = TECINI100((char*)"Flow", (char*)"X Y Z U V W UU P Cs", filen, (char*)".", &Debug, &VIsDouble);
		else if (levelset) I = TECINI100((char*)"Flow", (char*)"X Y Z U V W UU P Nv Level", filen, (char*)".", &Debug, &VIsDouble);
		else I = TECINI100((char*)"Flow", (char*)"X Y Z U V W UU P Nv", filen, (char*)".", &Debug, &VIsDouble);
	}

	for (bi=0; bi<block_number; bi++) {
		DA da = user[bi].da, fda = user[bi].fda;
		DALocalInfo info = user[bi].info;

		PetscInt	xs = info.xs, xe = info.xs + info.xm;
		PetscInt 	ys = info.ys, ye = info.ys + info.ym;
		PetscInt	zs = info.zs, ze = info.zs + info.zm;
		PetscInt	mx = info.mx, my = info.my, mz = info.mz;

		PetscInt	lxs, lys, lzs, lxe, lye, lze;
		PetscInt	i, j, k;
		PetscReal	***aj;
		Cmpnts	***ucat, ***coor, ***ucat_o, ***csi, ***eta, ***zet;
		PetscReal	***p, ***nvert, ***level;
		Vec		Coor, zCoor, nCoor;
		VecScatter	ctx;
		Vec K_Omega;

		DAVecGetArray(user[bi].da, user[bi].Nvert, &nvert);
		/*DAVecGetArray(user[bi].da, user[bi].Aj, &aj);
		DAVecGetArray(user[bi].fda, user[bi].Csi, &csi);
		DAVecGetArray(user[bi].fda, user[bi].Eta, &eta);
		DAVecGetArray(user[bi].fda, user[bi].Zet, &zet);*/
		DAVecGetArray(user[bi].fda, user[bi].Ucat, &ucat);

		INTEGER4	ZoneType=0, ICellMax=0, JCellMax=0, KCellMax=0;
		INTEGER4	IsBlock=1, NumFaceConnections=0, FaceNeighborMode=0;
		INTEGER4    ShareConnectivityFromZone=0;
		INTEGER4	LOC[40] = {1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0}; /* 1 is cell-centered 0 is node centered */

		/*************************/
		printf("mi=%d, mj=%d, mk=%d\n", mx, my, mz);
		printf("xs=%d, xe=%d\n", xs, xe);
		printf("ys=%d, ye=%d\n", ys, ye);
		printf("zs=%d, ze=%d\n", zs, ze);
		//exit(0);

		i_begin = 1, i_end = mx-1;	// cross section in tecplot
		j_begin = 1, j_end = my-1;
		k_begin = 1, k_end = mz-1;

		PetscOptionsGetInt(PETSC_NULL, "-i_begin", &i_begin, PETSC_NULL);
		PetscOptionsGetInt(PETSC_NULL, "-i_end", &i_end, PETSC_NULL);
		PetscOptionsGetInt(PETSC_NULL, "-j_begin", &j_begin, PETSC_NULL);
		PetscOptionsGetInt(PETSC_NULL, "-j_end", &j_end, PETSC_NULL);
		PetscOptionsGetInt(PETSC_NULL, "-k_begin", &k_begin, PETSC_NULL);
		PetscOptionsGetInt(PETSC_NULL, "-k_end", &k_end, PETSC_NULL);

		xs = i_begin - 1, xe = i_end+1;
		ys = j_begin - 1, ye = j_end+1;
		zs = k_begin - 1, ze = k_end+1;

		printf("xs=%d, xe=%d\n", xs, xe);
		printf("ys=%d, ye=%d\n", ys, ye);
		printf("zs=%d, ze=%d\n", zs, ze);
		//exit(0);
		//xs=0, xe=nsection+1;
		/*************************/

		if (vc) {
			LOC[3]=0; LOC[4]=0; LOC[5]=0; LOC[6]=0;
		}
		else if (only_V) {
			LOC[4]=0; LOC[5]=0; LOC[6]=0;
		}
		/*
		IMax = mx-1;
		JMax = my-1;
		KMax = mz-1;
		*/
		IMax = i_end - i_begin + 1;
		JMax = j_end - j_begin + 1;
		KMax = k_end - k_begin + 1;

		I = TECZNE100((char*)"Block 1",
			&ZoneType, 	/* Ordered zone */
			&IMax,
			&JMax,
			&KMax,
			&ICellMax,
			&JCellMax,
			&KCellMax,
			&IsBlock,	/* ISBLOCK  BLOCK format */
			&NumFaceConnections,
			&FaceNeighborMode,
			LOC,
			NULL,
			&ShareConnectivityFromZone); /* No connectivity sharing */

		//III = (mx-1) * (my-1) * (mz-1);
		III = IMax*JMax*KMax;

		DAGetCoordinates(da, &Coor);
		DAVecGetArray(fda, Coor, &coor);

		float *x;
		x = new float [III];

		for (k=zs; k<ze-1; k++)
		for (j=ys; j<ye-1; j++)
		for (i=xs; i<xe-1; i++) {
			x[(k-zs) * IMax*JMax + (j-ys)*IMax + (i-xs)] = coor[k][j][i].x;
		}
		I = TECDAT100(&III, &x[0], &DIsDouble);

		for (k=zs; k<ze-1; k++)
		for (j=ys; j<ye-1; j++)
		for (i=xs; i<xe-1; i++) {
			x[(k-zs) * IMax*JMax + (j-ys)*IMax + (i-xs)] = coor[k][j][i].y;
		}
		I = TECDAT100(&III, &x[0], &DIsDouble);

		for (k=zs; k<ze-1; k++)
		for (j=ys; j<ye-1; j++)
		for (i=xs; i<xe-1; i++) {
			x[(k-zs) * IMax*JMax + (j-ys)*IMax + (i-xs)] = coor[k][j][i].z;
		}
		I = TECDAT100(&III, &x[0], &DIsDouble);

		DAVecRestoreArray(fda, Coor, &coor);
		delete []x;

		if (!vc) {
			x = new float [(mx-1)*(my-1)*(mz-1)];
			DAVecGetArray(user[bi].fda, user[bi].Ucat_o, &ucat_o);
			for (k=0; k<mz-1; k++)
			for (j=0; j<my-1; j++)
			for (i=0; i<mx-1; i++) {
				ucat_o[k][j][i].x = 0.125 *
					(ucat[k][j][i].x + ucat[k][j][i+1].x +
					ucat[k][j+1][i].x + ucat[k][j+1][i+1].x +
					ucat[k+1][j][i].x + ucat[k+1][j][i+1].x +
					ucat[k+1][j+1][i].x + ucat[k+1][j+1][i+1].x);
				ucat_o[k][j][i].y = 0.125 *
					(ucat[k][j][i].y + ucat[k][j][i+1].y +
					ucat[k][j+1][i].y + ucat[k][j+1][i+1].y +
					ucat[k+1][j][i].y + ucat[k+1][j][i+1].y +
					ucat[k+1][j+1][i].y + ucat[k+1][j+1][i+1].y);
				ucat_o[k][j][i].z = 0.125 *
					(ucat[k][j][i].z + ucat[k][j][i+1].z +
					ucat[k][j+1][i].z + ucat[k][j+1][i+1].z +
					ucat[k+1][j][i].z + ucat[k+1][j][i+1].z +
					ucat[k+1][j+1][i].z + ucat[k+1][j+1][i+1].z);
			}

			for (k=zs; k<ze-1; k++)
			for (j=ys; j<ye-1; j++)
			for (i=xs; i<xe-1; i++) {
				x[k * (mx-1)*(my-1) + j*(mx-1) + i] = ucat_o[k][j][i].x;
			}
			if (!only_V) I = TECDAT100(&III, &x[0], &DIsDouble);

			for (k=zs; k<ze-1; k++)
			for (j=ys; j<ye-1; j++)
			for (i=xs; i<xe-1; i++) {
				x[k * (mx-1)*(my-1) + j*(mx-1) + i] = ucat_o[k][j][i].y;
			}
			if (!only_V) I = TECDAT100(&III, &x[0], &DIsDouble);

			for (k=zs; k<ze-1; k++)
			for (j=ys; j<ye-1; j++)
			for (i=xs; i<xe-1; i++) {
				x[k * (mx-1)*(my-1) + j*(mx-1) + i] = ucat_o[k][j][i].z;
			}

			if (!only_V) I = TECDAT100(&III, &x[0], &DIsDouble);

			for (k=zs; k<ze-1; k++)
			for (j=ys; j<ye-1; j++)
			for (i=xs; i<xe-1; i++) {
				x[k * (mx-1)*(my-1) + j*(mx-1) + i] = sqrt( ucat_o[k][j][i].x*ucat_o[k][j][i].x + ucat_o[k][j][i].y*ucat_o[k][j][i].y + ucat_o[k][j][i].z*ucat_o[k][j][i].z );
			}
			I = TECDAT100(&III, &x[0], &DIsDouble);

			DAVecRestoreArray(user[bi].fda, user[bi].Ucat_o, &ucat_o);
			delete []x;
		}
		else {
			//x = new float [(mx-2)*(my-2)*(mz-2)];
			//III = (mx-2) * (my-2) * (mz-2);
			III = (IMax-1)*(JMax-1)*(KMax-1);
			x = new float [III];

			if (!only_V)  {
				for (k=zs; k<ze-2; k++)
				for (j=ys; j<ye-2; j++)
				for (i=xs; i<xe-2; i++) {
					x[ (k-zs) * (IMax-1)*(JMax-1) + (j-ys) * (IMax-1) + (i-xs)] = ucat[k+1][j+1][i+1].x;
				}
				I = TECDAT100(&III, &x[0], &DIsDouble);

				for (k=zs; k<ze-2; k++)
				for (j=ys; j<ye-2; j++)
				for (i=xs; i<xe-2; i++) {
					x[ (k-zs) * (IMax-1)*(JMax-1) + (j-ys) * (IMax-1) + (i-xs)] = ucat[k+1][j+1][i+1].y;
				}
				I = TECDAT100(&III, &x[0], &DIsDouble);

				for (k=zs; k<ze-2; k++)
				for (j=ys; j<ye-2; j++)
				for (i=xs; i<xe-2; i++) {
					x[ (k-zs) * (IMax-1)*(JMax-1) + (j-ys) * (IMax-1) + (i-xs)] = ucat[k+1][j+1][i+1].z;
				}
				I = TECDAT100(&III, &x[0], &DIsDouble);
			}

			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) {
				x[ (k-zs) * (IMax-1)*(JMax-1) + (j-ys) * (IMax-1) + (i-xs)] = sqrt(ucat[k+1][j+1][i+1].x*ucat[k+1][j+1][i+1].x+ucat[k+1][j+1][i+1].y*ucat[k+1][j+1][i+1].y+ucat[k+1][j+1][i+1].z*ucat[k+1][j+1][i+1].z);
			}
			I = TECDAT100(&III, &x[0], &DIsDouble);
			delete []x;
		}


		III = (IMax-1)*(JMax-1)*(KMax-1);
		//III = (mx-2) * (my-2) * (mz-2);
	 		x = new float [III];
		//x.resize (III);

		if (!only_V) {
			DAVecGetArray(user[bi].da, user[bi].P, &p);
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) {
				x[ (k-zs) * (IMax-1)*(JMax-1) + (j-ys) * (IMax-1) + (i-xs)] = p[k+1][j+1][i+1];
			}
			I = TECDAT100(&III, &x[0], &DIsDouble);
			DAVecRestoreArray(user[bi].da, user[bi].P, &p);
		}


		if (only_V!=2) {
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) {
				x[ (k-zs) * (IMax-1)*(JMax-1) + (j-ys) * (IMax-1) + (i-xs)] = nvert[k+1][j+1][i+1];
			}
			I = TECDAT100(&III, &x[0], &DIsDouble);
		}

		if (only_V==2) {	// Z Vorticity
			/*
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) {
				double dudc, dvdc, dwdc, dude, dvde, dwde, dudz, dvdz, dwdz;
				double du_dx, du_dy, du_dz, dv_dx, dv_dy, dv_dz, dw_dx, dw_dy, dw_dz;
				double csi0 = csi[k][j][i].x, csi1 = csi[k][j][i].y, csi2 = csi[k][j][i].z;
				double eta0= eta[k][j][i].x, eta1 = eta[k][j][i].y, eta2 = eta[k][j][i].z;
				double zet0 = zet[k][j][i].x, zet1 = zet[k][j][i].y, zet2 = zet[k][j][i].z;
				double ajc = aj[k][j][i];

				if (i==0 || j==0 || k==0) x[k * (mx-2)*(my-2) + j*(mx-2) + i] = 0;
				else {
					Compute_du_center (i, j, k, mx, my, mz, ucat, nvert, &dudc, &dvdc, &dwdc, &dude, &dvde, &dwde, &dudz, &dvdz, &dwdz);
					Compute_du_dxyz (	csi0, csi1, csi2, eta0, eta1, eta2, zet0, zet1, zet2, ajc, dudc, dvdc, dwdc, dude, dvde, dwde, dudz, dvdz, dwdz,
										&du_dx, &dv_dx, &dw_dx, &du_dy, &dv_dy, &dw_dy, &du_dz, &dv_dz, &dw_dz );
					x[k * (mx-2)*(my-2) + j*(mx-2) + i] = dv_dx - du_dy;
				}
			}
			I = TECDAT100(&III, &x[0], &DIsDouble);
			*/
		}

		if (!onlyV && rans /*&& rans_output*/) {
			Cmpnts2 ***komega;
			DACreateGlobalVector(user->fda2, &K_Omega);
			PetscViewer	viewer;
			sprintf(filen, "kfield%05d_%1d.dat", ti, user->_this);
			PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
			VecLoadIntoVector(viewer, K_Omega);
			PetscViewerDestroy(viewer);
			DAVecGetArray(user[bi].fda2, K_Omega, &komega);

			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) x[ (k-zs) * (IMax-1)*(JMax-1) + (j-ys) * (IMax-1) + (i-xs)] = komega[k+1][j+1][i+1].x;
			I = TECDAT100(&III, &x[0], &DIsDouble);

			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) x[ (k-zs) * (IMax-1)*(JMax-1) + (j-ys) * (IMax-1) + (i-xs)] = komega[k+1][j+1][i+1].y;
			I = TECDAT100(&III, &x[0], &DIsDouble);

			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) x[ (k-zs) * (IMax-1)*(JMax-1) + (j-ys) * (IMax-1) + (i-xs)] = komega[k+1][j+1][i+1].x/(komega[k+1][j+1][i+1].y+1.e-20);
			I = TECDAT100(&III, &x[0], &DIsDouble);

			DAVecRestoreArray(user[bi].fda2, K_Omega, &komega);
			VecDestroy(K_Omega);
		}
		if (levelset) {
			PetscReal ***level;
			Vec Levelset;
			DACreateGlobalVector(user->da, &Levelset);
			PetscViewer	viewer;
			sprintf(filen, "lfield%05d_%1d.dat", ti, user->_this);
			PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
			VecLoadIntoVector(viewer, Levelset);
			PetscViewerDestroy(viewer);
			DAVecGetArray(user[bi].da, Levelset, &level);

			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) x[ (k-zs) * (IMax-1)*(JMax-1) + (j-ys) * (IMax-1) + (i-xs)] = level[k+1][j+1][i+1];
			I = TECDAT100(&III, &x[0], &DIsDouble);

			DAVecRestoreArray(user[bi].da, Levelset, &level);
			VecDestroy(Levelset);
		}

		delete []x;
		/*
		DAVecRestoreArray(user[bi].da, user[bi].Aj, &aj);
		DAVecRestoreArray(user[bi].fda, user[bi].Csi, &csi);
		DAVecRestoreArray(user[bi].fda, user[bi].Eta, &eta);
		DAVecRestoreArray(user[bi].fda, user[bi].Zet, &zet);*/
		DAVecRestoreArray(user[bi].fda, user[bi].Ucat, &ucat);
		DAVecRestoreArray(user[bi].da, user[bi].Nvert, &nvert);
	}
	I = TECEND100();
	return 0;
}
#else
PetscErrorCode TECIOOut_V(UserCtx *user, int only_V)
{
	PetscPrintf(PETSC_COMM_WORLD, "Compiled without Tecplot. Function TECIOOut_V not available!\n");
	return -1;
}
#endif


#ifdef TECIO
PetscErrorCode TECIOOut_Averaging(UserCtx *user)	// seokkoo
{
	PetscInt bi;

	char filen[80];
	sprintf(filen, "%sResult%05d-avg.plt", prefix, ti);

	INTEGER4 I, Debug, VIsDouble, DIsDouble, III, IMax, JMax, KMax;
	VIsDouble = 0;
	DIsDouble = 0;
	Debug = 0;

	if (pcr) I = TECINI100((char*)"Averaging", "X Y Z P Velocity_Magnitude Nv",  filen, (char*)".",  &Debug,  &VIsDouble);
	else if (avg==1) {

		if (averaging_option==1) {
			//I = TECINI100((char*)"Averaging", "X Y Z U V W uu vv ww uv vw uw UV_ VW_ UW_ Nv",  filen, (char*)".",  &Debug,  &VIsDouble); //OSL
			//I = TECINI100((char*)"Averaging", "X Y Z U V W uu vv ww uv vw uw UU_ VV_ WW_ Nv",  filen, (char*)".",  &Debug,  &VIsDouble); //OSL
			I = TECINI100((char*)"Averaging", "X Y Z U V W uu vv ww uv vw uw Nv",  filen, (char*)".",  &Debug,  &VIsDouble);
		}
		else if (averaging_option==2) {
			//I = TECINI100((char*)"Averaging", "X Y Z U V W uu vv ww uv vw uw  UV_ VW_ UW_ P pp Nv",  filen, (char*)".",  &Debug,  &VIsDouble);
			I = TECINI100((char*)"Averaging", "X Y Z U V W uu vv ww uv vw uw  P pp Nv",  filen, (char*)".",  &Debug,  &VIsDouble);
		}
		else if (averaging_option==3) {
			//I = TECINI100((char*)"Averaging", "X Y Z U V W uu vv ww uv vw uw UV_ VW_ UW_ P pp Vortx Vorty Vortz vortx2 vorty2 vortz2 Nv",  filen, (char*)".",  &Debug,  &VIsDouble);
			I = TECINI100((char*)"Averaging", "X Y Z U V W uu vv ww uv vw uw P pp Vortx Vorty Vortz vortx2 vorty2 vortz2 Nv",  filen, (char*)".",  &Debug,  &VIsDouble);
		}

	}
	else if (avg==2) I = TECINI100((char*)"Averaging", "X Y Z U V W K Nv",  filen, (char*)".",  &Debug,  &VIsDouble);


	for (bi=0; bi<block_number; bi++) {
		DA da = user[bi].da, fda = user[bi].fda;
		DALocalInfo info = user[bi].info;

		PetscInt	xs = info.xs, xe = info.xs + info.xm;
		PetscInt  	ys = info.ys, ye = info.ys + info.ym;
		PetscInt	zs = info.zs, ze = info.zs + info.zm;
		PetscInt	mx = info.mx, my = info.my, mz = info.mz;

		PetscInt	lxs, lys, lzs, lxe, lye, lze;
		PetscInt	i, j, k;
		PetscReal	***aj;
		Cmpnts	***ucat, ***coor, ***ucat_o;
		Cmpnts	***u2sum, ***u1sum,  ***usum;
		PetscReal	***p, ***nvert;
		Vec		Coor, zCoor, nCoor;
		//VecScatter ctx;

		Vec X, Y, Z, U, V, W;

		INTEGER4	ZoneType=0, ICellMax=0, JCellMax=0, KCellMax=0;
		INTEGER4	IsBlock=1, NumFaceConnections=0, FaceNeighborMode=0;
		INTEGER4    ShareConnectivityFromZone=0;
		INTEGER4	LOC[100] = {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		/* 1 is cell-centered   0 is node centered */

		IMax = mx-1; JMax = my-1; KMax = mz-1;

		I = TECZNE100((char*)"Block 1",
			&ZoneType, 	/* Ordered zone */
			&IMax,
			&JMax,
			&KMax,
			&ICellMax,
			&JCellMax,
			&KCellMax,
			&IsBlock,	/* ISBLOCK  BLOCK format */
			&NumFaceConnections,
			&FaceNeighborMode,
			LOC,
			NULL,
			&ShareConnectivityFromZone); /* No connectivity sharing */

		float *x;

		x = new float [(mx-1)*(my-1)*(mz-1)];
		III = (mx-1) * (my-1) * (mz-1);

		DAGetCoordinates(da, &Coor);
		DAVecGetArray(fda, Coor, &coor);

		// X
		for (k=zs; k<ze-1; k++)
		for (j=ys; j<ye-1; j++)
		for (i=xs; i<xe-1; i++) x[k * (mx-1)*(my-1) + j*(mx-1) + i] = coor[k][j][i].x;
		I = TECDAT100(&III, x, &DIsDouble);

		// Y
		for (k=zs; k<ze-1; k++)
		for (j=ys; j<ye-1; j++)
		for (i=xs; i<xe-1; i++) x[k * (mx-1)*(my-1) + j*(mx-1) + i] = coor[k][j][i].y;
		I = TECDAT100(&III, x, &DIsDouble);

		// Z
		for (k=zs; k<ze-1; k++)
		for (j=ys; j<ye-1; j++)
		for (i=xs; i<xe-1; i++) x[k * (mx-1)*(my-1) + j*(mx-1) + i] = coor[k][j][i].z;
		I = TECDAT100(&III, x, &DIsDouble);

		DAVecRestoreArray(fda, Coor, &coor);

		//delete []x;
		double N=(double)tis+1.0;
		//x = new float [(mx-2)*(my-2)*(mz-2)];

		III = (mx-2) * (my-2) * (mz-2);

		if (pcr)  {
			DAVecGetArray(user[bi].da, user[bi].P, &p);
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) x[k * (mx-2)*(my-2) + j*(mx-2) + i] = p[k+1][j+1][i+1];
			I = TECDAT100(&III, x, &DIsDouble);
			DAVecRestoreArray(user[bi].da, user[bi].P, &p);

			// Load ucat
			PetscViewer	viewer;
			sprintf(filen, "ufield%05d_%1d.dat", ti, user->_this);
			PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
			VecLoadIntoVector(viewer, (user[bi].Ucat));
			PetscViewerDestroy(viewer);

			DAVecGetArray(user[bi].fda, user[bi].Ucat, &ucat);
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) x[k * (mx-2)*(my-2) + j*(mx-2) + i]
								= sqrt ( ucat[k+1][j+1][i+1].x*ucat[k+1][j+1][i+1].x + ucat[k+1][j+1][i+1].y*ucat[k+1][j+1][i+1].y + ucat[k+1][j+1][i+1].z*ucat[k+1][j+1][i+1].z );
			I = TECDAT100(&III, x, &DIsDouble);
			DAVecRestoreArray(user[bi].fda, user[bi].Ucat, &ucat);
		}
		else if (avg==1) {
			PetscViewer viewer;
			char filen[128];

			DACreateGlobalVector(user[bi].fda, &user[bi].Ucat_sum);
			DACreateGlobalVector(user[bi].fda, &user[bi].Ucat_cross_sum);
			DACreateGlobalVector(user[bi].fda, &user[bi].Ucat_square_sum);

			sprintf(filen, "su0_%05d_%1d.dat", ti, user[bi]._this);
			PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
			VecLoadIntoVector(viewer, (user[bi].Ucat_sum));
			PetscViewerDestroy(viewer);

			sprintf(filen, "su1_%05d_%1d.dat", ti, user[bi]._this);
			PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
			VecLoadIntoVector(viewer, (user[bi].Ucat_cross_sum));
			PetscViewerDestroy(viewer);

			sprintf(filen, "su2_%05d_%1d.dat", ti, user[bi]._this);
			PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
			VecLoadIntoVector(viewer, (user[bi].Ucat_square_sum));
			PetscViewerDestroy(viewer);

			DAVecGetArray(user[bi].fda, user[bi].Ucat_sum, &usum);
			DAVecGetArray(user[bi].fda, user[bi].Ucat_cross_sum, &u1sum);
			DAVecGetArray(user[bi].fda, user[bi].Ucat_square_sum, &u2sum);

			// U
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) x[k * (mx-2)*(my-2) + j*(mx-2) + i] = usum[k+1][j+1][i+1].x/N;
			if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			I = TECDAT100(&III, x, &DIsDouble);

			// V
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) x[k * (mx-2)*(my-2) + j*(mx-2) + i] = usum[k+1][j+1][i+1].y/N;
			if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			I = TECDAT100(&III, x, &DIsDouble);

			// W
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) x[k * (mx-2)*(my-2) + j*(mx-2) + i] = usum[k+1][j+1][i+1].z/N;
			if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			I = TECDAT100(&III, x, &DIsDouble);

			// uu, u rms
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) {
				double U = usum[k+1][j+1][i+1].x/N;
				x[k * (mx-2)*(my-2) + j*(mx-2) + i] = ( u2sum[k+1][j+1][i+1].x/N - U*U );
			}
			if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			I = TECDAT100(&III, x, &DIsDouble);

			// vv, v rms
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) {
				double V = usum[k+1][j+1][i+1].y/N;
				x[k * (mx-2)*(my-2) + j*(mx-2) + i] = ( u2sum[k+1][j+1][i+1].y/N - V*V );
			}
			if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			I = TECDAT100(&III, x, &DIsDouble);

			// ww, w rms
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) {
				double W = usum[k+1][j+1][i+1].z/N;
				x[k * (mx-2)*(my-2) + j*(mx-2) + i] = ( u2sum[k+1][j+1][i+1].z/N - W*W );
			}
			if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			I = TECDAT100(&III, x, &DIsDouble);

			// uv
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) {
				double UV = usum[k+1][j+1][i+1].x*usum[k+1][j+1][i+1].y / (N*N);
				x[k * (mx-2)*(my-2) + j*(mx-2) + i] = u1sum[k+1][j+1][i+1].x/N - UV;
			}
			if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			I = TECDAT100(&III, x, &DIsDouble);

			// vw
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) {
				double VW = usum[k+1][j+1][i+1].y*usum[k+1][j+1][i+1].z / (N*N);
				x[k * (mx-2)*(my-2) + j*(mx-2) + i] = u1sum[k+1][j+1][i+1].y/N - VW;
			}
			if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			I = TECDAT100(&III, x, &DIsDouble);

			// wu
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) {
				double WU = usum[k+1][j+1][i+1].z*usum[k+1][j+1][i+1].x / (N*N);
				x[k * (mx-2)*(my-2) + j*(mx-2) + i] = u1sum[k+1][j+1][i+1].z/N - WU;
			}
			if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			I = TECDAT100(&III, x, &DIsDouble);

			/*******************************
			//
			DACreateGlobalVector(user[bi].fda, &user[bi].Csi);
			DACreateGlobalVector(user[bi].fda, &user[bi].Eta);
			DACreateGlobalVector(user[bi].fda, &user[bi].Zet);
			DACreateGlobalVector(user[bi].da, &user[bi].Aj);
			FormMetrics(&(user[bi]));

			Cmpnts ***csi, ***eta, ***zet;
			DAVecGetArray(user[bi].fda, user[bi].Csi, &csi);
			DAVecGetArray(user[bi].fda, user[bi].Eta, &eta);
			DAVecGetArray(user[bi].fda, user[bi].Zet, &zet);

			//UV_ or UU_
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) {
				double U = usum[k+1][j+1][i+1].x/N;
				double V = usum[k+1][j+1][i+1].y/N;
				double W = usum[k+1][j+1][i+1].z/N;

				double uu = ( u2sum[k+1][j+1][i+1].x/N - U*U );
				double vv = ( u2sum[k+1][j+1][i+1].y/N - V*V );
				double ww = ( u2sum[k+1][j+1][i+1].z/N - W*W );
				double uv = u1sum[k+1][j+1][i+1].x/N - U*V;
				double vw = u1sum[k+1][j+1][i+1].y/N - V*W;
				double uw = u1sum[k+1][j+1][i+1].z/N - W*U;

				double csi0 = csi[k+1][j+1][i+1].x, csi1 = csi[k+1][j+1][i+1].y, csi2 = csi[k+1][j+1][i+1].z;
				double eta0 = eta[k+1][j+1][i+1].x, eta1 = eta[k+1][j+1][i+1].y, eta2 = eta[k+1][j+1][i+1].z;
				double zet0 = zet[k+1][j+1][i+1].x, zet1 = zet[k+1][j+1][i+1].y, zet2 = zet[k+1][j+1][i+1].z;

				// UV_
				//x[k * (mx-2)*(my-2) + j*(mx-2) + i] = Contravariant_Reynolds_stress(uu, uv, uw, vv, vw, ww,	csi0, csi1, csi2, eta0, eta1, eta2);
				// UU_
				x[k * (mx-2)*(my-2) + j*(mx-2) + i] = Contravariant_Reynolds_stress(uu, uv, uw, vv, vw, ww,	csi0, csi1, csi2, csi0, csi1, csi2);
			}
			if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			I = TECDAT100(&III, x, &DIsDouble);

			// VW_ or VV_
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) {
				double U = usum[k+1][j+1][i+1].x/N;
				double V = usum[k+1][j+1][i+1].y/N;
				double W = usum[k+1][j+1][i+1].z/N;

				double uu = ( u2sum[k+1][j+1][i+1].x/N - U*U );
				double vv = ( u2sum[k+1][j+1][i+1].y/N - V*V );
				double ww = ( u2sum[k+1][j+1][i+1].z/N - W*W );
				double uv = u1sum[k+1][j+1][i+1].x/N - U*V;
				double vw = u1sum[k+1][j+1][i+1].y/N - V*W;
				double uw = u1sum[k+1][j+1][i+1].z/N - W*U;

				double csi0 = csi[k+1][j+1][i+1].x, csi1 = csi[k+1][j+1][i+1].y, csi2 = csi[k+1][j+1][i+1].z;
				double eta0 = eta[k+1][j+1][i+1].x, eta1 = eta[k+1][j+1][i+1].y, eta2 = eta[k+1][j+1][i+1].z;
				double zet0 = zet[k+1][j+1][i+1].x, zet1 = zet[k+1][j+1][i+1].y, zet2 = zet[k+1][j+1][i+1].z;

				// VW_
				//x[k * (mx-2)*(my-2) + j*(mx-2) + i] = Contravariant_Reynolds_stress(uu, uv, uw, vv, vw, ww,	zet0, zet1, zet2, eta0, eta1, eta2);
				// VV_
				x[k * (mx-2)*(my-2) + j*(mx-2) + i] = Contravariant_Reynolds_stress(uu, uv, uw, vv, vw, ww,	eta0, eta1, eta2, eta0, eta1, eta2);
			}
			if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			I = TECDAT100(&III, x, &DIsDouble);

			//UW_ or WW_
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) {
				double U = usum[k+1][j+1][i+1].x/N;
				double V = usum[k+1][j+1][i+1].y/N;
				double W = usum[k+1][j+1][i+1].z/N;

				double uu = ( u2sum[k+1][j+1][i+1].x/N - U*U );
				double vv = ( u2sum[k+1][j+1][i+1].y/N - V*V );
				double ww = ( u2sum[k+1][j+1][i+1].z/N - W*W );
				double uv = u1sum[k+1][j+1][i+1].x/N - U*V;
				double vw = u1sum[k+1][j+1][i+1].y/N - V*W;
				double uw = u1sum[k+1][j+1][i+1].z/N - W*U;

				double csi0 = csi[k+1][j+1][i+1].x, csi1 = csi[k+1][j+1][i+1].y, csi2 = csi[k+1][j+1][i+1].z;
				double eta0 = eta[k+1][j+1][i+1].x, eta1 = eta[k+1][j+1][i+1].y, eta2 = eta[k+1][j+1][i+1].z;
				double zet0 = zet[k+1][j+1][i+1].x, zet1 = zet[k+1][j+1][i+1].y, zet2 = zet[k+1][j+1][i+1].z;

				// UW_
				//x[k * (mx-2)*(my-2) + j*(mx-2) + i] = Contravariant_Reynolds_stress(uu, uv, uw, vv, vw, ww,	csi0, csi1, csi2, zet0, zet1, zet2);
				// WW_
				x[k * (mx-2)*(my-2) + j*(mx-2) + i] = Contravariant_Reynolds_stress(uu, uv, uw, vv, vw, ww,	zet0, zet1, zet2, zet0, zet1, zet2);
			}
			if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			I = TECDAT100(&III, x, &DIsDouble);

			DAVecRestoreArray(user[bi].fda, user[bi].Csi, &csi);
			DAVecRestoreArray(user[bi].fda, user[bi].Eta, &eta);
			DAVecRestoreArray(user[bi].fda, user[bi].Zet, &zet);

			VecDestroy(user[bi].Csi);
			VecDestroy(user[bi].Eta);
			VecDestroy(user[bi].Zet);
			VecDestroy(user[bi].Aj);

			DAVecRestoreArray(user[bi].fda, user[bi].Ucat_sum, &usum);
			DAVecRestoreArray(user[bi].fda, user[bi].Ucat_cross_sum, &u1sum);
			DAVecRestoreArray(user[bi].fda, user[bi].Ucat_square_sum, &u2sum);

			VecDestroy(user[bi].Ucat_sum);
			VecDestroy(user[bi].Ucat_cross_sum);
			VecDestroy(user[bi].Ucat_square_sum);
			//
			********************************/

			if (averaging_option>=2) {
				Vec P_sum, P_square_sum;
				PetscReal ***psum, ***p2sum;

				DACreateGlobalVector(user[bi].da, &P_sum);
				DACreateGlobalVector(user[bi].da, &P_square_sum);

				sprintf(filen, "sp_%05d_%1d.dat", ti, user[bi]._this);
				PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
				VecLoadIntoVector(viewer, P_sum);
				PetscViewerDestroy(viewer);

				sprintf(filen, "sp2_%05d_%1d.dat", ti, user[bi]._this);
				PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
				VecLoadIntoVector(viewer, P_square_sum);
				PetscViewerDestroy(viewer);

				DAVecGetArray(user[bi].da, P_sum, &psum);
				DAVecGetArray(user[bi].da, P_square_sum, &p2sum);

				for (k=zs; k<ze-2; k++)
				for (j=ys; j<ye-2; j++)
				for (i=xs; i<xe-2; i++) {
					double P = psum[k+1][j+1][i+1]/N;
					x[k * (mx-2)*(my-2) + j*(mx-2) + i] = P;
				}
				if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				I = TECDAT100(&III, x, &DIsDouble);


				for (k=zs; k<ze-2; k++)
				for (j=ys; j<ye-2; j++)
				for (i=xs; i<xe-2; i++) {
					double P = psum[k+1][j+1][i+1]/N;
					x[k * (mx-2)*(my-2) + j*(mx-2) + i] = ( p2sum[k+1][j+1][i+1]/N - P*P );
				}
				if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				I = TECDAT100(&III, x, &DIsDouble);

				DAVecRestoreArray(user[bi].da, P_sum, &psum);
				DAVecRestoreArray(user[bi].da, P_square_sum, &p2sum);

				VecDestroy(P_sum);
				VecDestroy(P_square_sum);
			}

			if (averaging_option>=3) {
				Vec Vort_sum, Vort_square_sum;
				Cmpnts ***vortsum, ***vort2sum;

				DACreateGlobalVector(user[bi].fda, &Vort_sum);
				DACreateGlobalVector(user[bi].fda, &Vort_square_sum);

				sprintf(filen, "svo_%05d_%1d.dat", ti, user[bi]._this);
				PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
				VecLoadIntoVector(viewer, Vort_sum);
				PetscViewerDestroy(viewer);

				sprintf(filen, "svo2_%05d_%1d.dat", ti, user[bi]._this);
				PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
				VecLoadIntoVector(viewer, Vort_square_sum);
				PetscViewerDestroy(viewer);

				DAVecGetArray(user[bi].fda, Vort_sum, &vortsum);
				DAVecGetArray(user[bi].fda, Vort_square_sum, &vort2sum);

				for (k=zs; k<ze-2; k++)
				for (j=ys; j<ye-2; j++)
				for (i=xs; i<xe-2; i++) {
					double vortx = vortsum[k+1][j+1][i+1].x/N;
						x[k * (mx-2)*(my-2) + j*(mx-2) + i] = vortx;
				}
				if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				I = TECDAT100(&III, x, &DIsDouble);

				for (k=zs; k<ze-2; k++)
				for (j=ys; j<ye-2; j++)
				for (i=xs; i<xe-2; i++) {
					double vorty = vortsum[k+1][j+1][i+1].y/N;
					x[k * (mx-2)*(my-2) + j*(mx-2) + i] = vorty;
				}
				if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				I = TECDAT100(&III, x, &DIsDouble);

				for (k=zs; k<ze-2; k++)
				for (j=ys; j<ye-2; j++)
				for (i=xs; i<xe-2; i++) {
					double vortz = vortsum[k+1][j+1][i+1].z/N;
					x[k * (mx-2)*(my-2) + j*(mx-2) + i] = vortz;
				}
				if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				I = TECDAT100(&III, x, &DIsDouble);

				for (k=zs; k<ze-2; k++)
				for (j=ys; j<ye-2; j++)
				for (i=xs; i<xe-2; i++) {
					double vortx = vortsum[k+1][j+1][i+1].x/N;
					x[k * (mx-2)*(my-2) + j*(mx-2) + i] = ( vort2sum[k+1][j+1][i+1].x/N - vortx*vortx );
				}
				if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				I = TECDAT100(&III, x, &DIsDouble);

				for (k=zs; k<ze-2; k++)
				for (j=ys; j<ye-2; j++)
				for (i=xs; i<xe-2; i++) {
					double vorty = vortsum[k+1][j+1][i+1].y/N;
					x[k * (mx-2)*(my-2) + j*(mx-2) + i] = ( vort2sum[k+1][j+1][i+1].y/N - vorty*vorty );
				}
				if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				I = TECDAT100(&III, x, &DIsDouble);

				for (k=zs; k<ze-2; k++)
				for (j=ys; j<ye-2; j++)
				for (i=xs; i<xe-2; i++) {
					double vortz = vortsum[k+1][j+1][i+1].z/N;
					x[k * (mx-2)*(my-2) + j*(mx-2) + i] = ( vort2sum[k+1][j+1][i+1].z/N - vortz*vortz );
				}
				if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
				I = TECDAT100(&III, x, &DIsDouble);

				DAVecRestoreArray(user[bi].fda, Vort_sum, &vortsum);
				DAVecRestoreArray(user[bi].fda, Vort_square_sum, &vort2sum);

				VecDestroy(Vort_sum);
				VecDestroy(Vort_square_sum);

				//haha
				/*
				//TKE budget
			 	Vec Udp_sum, dU2_sum, UUU_sum;
				PetscReal ***udpsum, ***aj;
				Cmpnts ***du2sum, ***uuusum;
				Cmpnts ***csi, ***eta, ***zet;

				DACreateGlobalVector(user[bi].da, &Udp_sum);
				DACreateGlobalVector(user[bi].fda, &dU2_sum);
				DACreateGlobalVector(user[bi].fda, &UUU_sum);
				DACreateGlobalVector(user[bi].fda, &user[bi].Ucat_sum);
				DACreateGlobalVector(user[bi].fda, &user[bi].Ucat_cross_sum);
				DACreateGlobalVector(user[bi].fda, &user[bi].Ucat_square_sum);
				DACreateGlobalVector(user[bi].fda, &user[bi].Csi);
				DACreateGlobalVector(user[bi].fda, &user[bi].Eta);
				DACreateGlobalVector(user[bi].fda, &user[bi].Zet);
				DACreateGlobalVector(user[bi].da, &user[bi].Aj);


				sprintf(filen, "su0_%05d_%1d.dat", ti, user[bi]._this);
				PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
				VecLoadIntoVector(viewer, (user[bi].Ucat_sum));
				PetscViewerDestroy(viewer);

				sprintf(filen, "su1_%05d_%1d.dat", ti, user[bi]._this);
				PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
				VecLoadIntoVector(viewer, (user[bi].Ucat_cross_sum));
				PetscViewerDestroy(viewer);

				sprintf(filen, "su2_%05d_%1d.dat", ti, user[bi]._this);
				PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
				VecLoadIntoVector(viewer, (user[bi].Ucat_square_sum));
				PetscViewerDestroy(viewer);

				sprintf(filen, "su3_%05d_%1d.dat", ti, user[bi]._this);
				PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
				VecLoadIntoVector(viewer, Udp_sum);
				PetscViewerDestroy(viewer);

				sprintf(filen, "su4_%05d_%1d.dat", ti, user[bi]._this);
				PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
				VecLoadIntoVector(viewer, dU2_sum);
				PetscViewerDestroy(viewer);

				sprintf(filen, "su5_%05d_%1d.dat", ti, user[bi]._this);
				PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
				VecLoadIntoVector(viewer, UUU_sum);
				PetscViewerDestroy(viewer);

				FormMetrics(&(user[bi]));

				DAVecGetArray(user[bi].da, user[bi].Aj, &aj);
				DAVecGetArray(user[bi].fda, user[bi].Csi, &csi);
				DAVecGetArray(user[bi].fda, user[bi].Eta, &eta);
				DAVecGetArray(user[bi].fda, user[bi].Zet, &zet);
				DAVecGetArray(user[bi].fda, user[bi].Ucat_sum, &usum);
				DAVecGetArray(user[bi].fda, user[bi].Ucat_cross_sum, &u1sum);
				DAVecGetArray(user[bi].fda, user[bi].Ucat_square_sum, &u2sum);
				DAVecGetArray(user[bi].da, Udp_sum, &udpsum);
				DAVecGetArray(user[bi].fda, dU2_sum, &du2sum);
				DAVecGetArray(user[bi].fda, UUU_sum, &uuusum);

				DAVecRestoreArray(user[bi].da, user[bi].Aj, &aj);
				DAVecRestoreArray(user[bi].fda, user[bi].Csi, &csi);
				DAVecRestoreArray(user[bi].fda, user[bi].Eta, &eta);
				DAVecRestoreArray(user[bi].fda, user[bi].Zet, &zet);
				DAVecRestoreArray(user[bi].fda, user[bi].Ucat_sum, &usum);
				DAVecRestoreArray(user[bi].fda, user[bi].Ucat_cross_sum, &u1sum);
				DAVecRestoreArray(user[bi].fda, user[bi].Ucat_square_sum, &u2sum);
				DAVecRestoreArray(user[bi].da, Udp_sum, &udpsum);
				DAVecRestoreArray(user[bi].fda, dU2_sum, &du2sum);
				DAVecRestoreArray(user[bi].fda, UUU_sum, &uuusum);

				VecDestroy(user[bi].Csi);
				VecDestroy(user[bi].Eta);
				VecDestroy(user[bi].Zet);
				VecDestroy(user[bi].Aj);
				VecDestroy(user[bi].Ucat_sum);
				VecDestroy(user[bi].Ucat_cross_sum);
				VecDestroy(user[bi].Ucat_square_sum);
				VecDestroy(Udp_sum);
				VecDestroy(dU2_sum);
				VecDestroy(UUU_sum);
				*/
			}
		}
		else if (avg==2) {
			PetscViewer viewer;
			Vec K_sum;
			PetscReal ***ksum;
			char filen[128];

			DACreateGlobalVector(user->fda, &user->Ucat_sum);
						            DACreateGlobalVector(user->fda, &user->Ucat_square_sum);
			if (rans) {
				DACreateGlobalVector(user->da, &K_sum);
			}

			sprintf(filen, "su0_%05d_%1d.dat", ti, user->_this);
			PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
			VecLoadIntoVector(viewer, (user->Ucat_sum));
			PetscViewerDestroy(viewer);

			if (rans) {
				sprintf(filen, "sk_%05d_%1d.dat", ti, user->_this);
				PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
				VecLoadIntoVector(viewer, K_sum);
				PetscViewerDestroy(viewer);
			}
			else {
				sprintf(filen, "su2_%05d_%1d.dat", ti, user->_this);
				PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
				VecLoadIntoVector(viewer, (user->Ucat_square_sum));
				PetscViewerDestroy(viewer);
			}

			DAVecGetArray(user[bi].fda, user[bi].Ucat_sum, &usum);

			if (rans) DAVecGetArray(user[bi].da, K_sum, &ksum);
			else DAVecGetArray(user[bi].fda, user[bi].Ucat_square_sum, &u2sum);

			// U
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) x[k * (mx-2)*(my-2) + j*(mx-2) + i] = usum[k+1][j+1][i+1].x/N;
			if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			I = TECDAT100(&III, x, &DIsDouble);

			// V
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) x[k * (mx-2)*(my-2) + j*(mx-2) + i] = usum[k+1][j+1][i+1].y/N;
			if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			I = TECDAT100(&III, x, &DIsDouble);

			// W
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) x[k * (mx-2)*(my-2) + j*(mx-2) + i] = usum[k+1][j+1][i+1].z/N;
			if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			I = TECDAT100(&III, x, &DIsDouble);

			// k
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) {
				if (rans)  {
					x[k * (mx-2)*(my-2) + j*(mx-2) + i] = ksum[k+1][j+1][i+1]/N;
				}
				else {
					double U = usum[k+1][j+1][i+1].x/N;
					double V = usum[k+1][j+1][i+1].y/N;
					double W = usum[k+1][j+1][i+1].z/N;
					x[k * (mx-2)*(my-2) + j*(mx-2) + i] = ( u2sum[k+1][j+1][i+1].x/N - U*U ) + ( u2sum[k+1][j+1][i+1].y/N - V*V ) + ( u2sum[k+1][j+1][i+1].z/N - W*W );
					x[k * (mx-2)*(my-2) + j*(mx-2) + i] *= 0.5;
				}
			}
			if (i_average) Iavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (j_average) Javg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (k_average) Kavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ik_average) IKavg(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			if (ikc_average) IKavg_c(x, xs, xe, ys, ye, zs, ze, mx, my, mz);
			I = TECDAT100(&III, x, &DIsDouble);

			DAVecRestoreArray(user[bi].fda, user[bi].Ucat_sum, &usum);

			if (rans) DAVecRestoreArray(user[bi].da, K_sum, &ksum);
			else DAVecRestoreArray(user[bi].fda, user[bi].Ucat_square_sum, &u2sum);

			VecDestroy(user->Ucat_sum);
			if (rans) VecDestroy(K_sum);
			else VecDestroy(user->Ucat_square_sum);
		}

		DAVecGetArray(user[bi].da, user[bi].Nvert, &nvert);
		for (k=zs; k<ze-2; k++)
		for (j=ys; j<ye-2; j++)
		for (i=xs; i<xe-2; i++) x[k * (mx-2)*(my-2) + j*(mx-2) + i] = nvert[k+1][j+1][i+1];
		I = TECDAT100(&III, x, &DIsDouble);
		DAVecRestoreArray(user[bi].da, user[bi].Nvert, &nvert);

		delete []x;
	}
	I = TECEND100();
	return 0;
}
#else
PetscErrorCode TECIOOut_Averaging(UserCtx *user)
{
	PetscPrintf(PETSC_COMM_WORLD, "Compiled without Tecplot. Function TECIOOut_Averaging not available!\n");
	return -1;
}
#endif

#ifdef TECIO
PetscErrorCode TECIOOutQ(UserCtx *user, int Q)
{
	PetscInt bi;

	char filen[80];
	sprintf(filen, "QCriteria%05d.plt", ti);

	INTEGER4 I, Debug, VIsDouble, DIsDouble, III, IMax, JMax, KMax;
	VIsDouble = 0;
	DIsDouble = 0;
	Debug = 0;

	if (Q==1) {
		printf("qcr=%d, Q-Criterion\n", Q);
		//I = TECINI100((char*)"Result", (char*)"X Y Z Q Velocity_Magnitude Nv",   filen,  (char*)".",   &Debug,  &VIsDouble);
		I = TECINI100((char*)"Result", (char*)"X Y Z Q Velocity_Magnitude",   filen,  (char*)".",   &Debug,  &VIsDouble);
	}
	else if (Q==2) {
		printf("Lambda2-Criterion\n");
		//I = TECINI100((char*)"Result", (char*)"X Y Z Lambda2 Velocity_Magnitude Nv",   filen,  (char*)".",   &Debug,  &VIsDouble);
		I = TECINI100((char*)"Result", (char*)"X Y Z Lambda2 Velocity_Magnitude",   filen,  (char*)".",   &Debug,  &VIsDouble);
	}
	else if (Q==3) {
		printf("Q-Criterion from saved file\n");
		I = TECINI100((char*)"Result", (char*)"X Y Z Q Velocity_Magnitude",   filen,  (char*)".",   &Debug,  &VIsDouble);
	}

	for (bi=0; bi<block_number; bi++) {
		DA da = user[bi].da, fda = user[bi].fda;
		DALocalInfo info = user[bi].info;

		PetscInt	xs = info.xs, xe = info.xs + info.xm;
		PetscInt  	ys = info.ys, ye = info.ys + info.ym;
		PetscInt	zs = info.zs, ze = info.zs + info.zm;
		PetscInt	mx = info.mx, my = info.my, mz = info.mz;

		PetscInt	lxs, lys, lzs, lxe, lye, lze;
		PetscInt	i, j, k;
		PetscReal	***aj;
		Cmpnts	***ucat, ***coor, ***ucat_o;
		PetscReal	***p, ***nvert;
		Vec		Coor, zCoor, nCoor;
		VecScatter	ctx;

		Vec X, Y, Z, U, V, W;

		INTEGER4	ZoneType=0, ICellMax=0, JCellMax=0, KCellMax=0;
		INTEGER4	IsBlock=1, NumFaceConnections=0, FaceNeighborMode=0;
		INTEGER4    ShareConnectivityFromZone=0;
		INTEGER4	LOC[8] = {1, 1, 1, 0, 0, 0, 0}; /* 1 is cell-centered 0 is node centered */

		IMax = mx-1; JMax = my-1; KMax = mz-1;

		I = TECZNE100((char*)"Block 1",
			&ZoneType, 	/* Ordered zone */
			&IMax,
			&JMax,
			&KMax,
			&ICellMax,
			&JCellMax,
			&KCellMax,
			&IsBlock,	/* ISBLOCK  BLOCK format */
			&NumFaceConnections,
			&FaceNeighborMode,
			LOC,
			NULL,
			&ShareConnectivityFromZone); /* No connectivity sharing */

		III = (mx-1) * (my-1) * (mz-1);
		float	*x;
		PetscMalloc(mx*my*mz*sizeof(float), &x);	// seokkoo

		DAGetCoordinates(da, &Coor);
		DAVecGetArray(fda, Coor, &coor);

		for (k=zs; k<ze-1; k++)
		for (j=ys; j<ye-1; j++)
		for (i=xs; i<xe-1; i++) {
			x[k * (mx-1)*(my-1) + j*(mx-1) + i] = coor[k][j][i].x;
		}
		I = TECDAT100(&III, x, &DIsDouble);

		for (k=zs; k<ze-1; k++)
		for (j=ys; j<ye-1; j++)
		for (i=xs; i<xe-1; i++) {
			x[k * (mx-1)*(my-1) + j*(mx-1) + i] = coor[k][j][i].y;
		}
		I = TECDAT100(&III, x, &DIsDouble);

		for (k=zs; k<ze-1; k++)
		for (j=ys; j<ye-1; j++)
		for (i=xs; i<xe-1; i++) {
			x[k * (mx-1)*(my-1) + j*(mx-1) + i] = coor[k][j][i].z;
		}

		I = TECDAT100(&III, x, &DIsDouble);
		DAVecRestoreArray(fda, Coor, &coor);

		III = (mx-2) * (my-2) * (mz-2);

		if (Q==1) {
			QCriteria(user);
			DAVecGetArray(user[bi].da, user[bi].P, &p);
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) {
				x[k * (mx-2)*(my-2) + j*(mx-2) + i] =p[k+1][j+1][i+1];
			}
			I = TECDAT100(&III, x, &DIsDouble);
			DAVecRestoreArray(user[bi].da, user[bi].P, &p);
		}
		else if (Q==2) {
			Lambda2(user);
			DAVecGetArray(user[bi].da, user[bi].P, &p);
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) {
				x[k * (mx-2)*(my-2) + j*(mx-2) + i] = p[k+1][j+1][i+1];
			}
			I = TECDAT100(&III, x, &DIsDouble);
			DAVecRestoreArray(user[bi].da, user[bi].P, &p);
		}
		else if (Q==3) {
			char filen2[128];
			PetscViewer	viewer;

			sprintf(filen2, "qfield%05d_%1d.dat", ti, user->_this);
			PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen2, FILE_MODE_READ, &viewer);
			VecLoadIntoVector(viewer, (user[bi].P));
			PetscViewerDestroy(viewer);

			DAVecGetArray(user[bi].da, user[bi].P, &p);
			for (k=zs; k<ze-2; k++)
			for (j=ys; j<ye-2; j++)
			for (i=xs; i<xe-2; i++) {
				x[k * (mx-2)*(my-2) + j*(mx-2) + i] = p[k+1][j+1][i+1];
			}
			I = TECDAT100(&III, x, &DIsDouble);
			DAVecRestoreArray(user[bi].da, user[bi].P, &p);
		}

		Velocity_Magnitude(user);
		DAVecGetArray(user[bi].da, user[bi].P, &p);
		for (k=zs; k<ze-2; k++)
		for (j=ys; j<ye-2; j++)
		for (i=xs; i<xe-2; i++) {
			x[k * (mx-2)*(my-2) + j*(mx-2) + i] = p[k+1][j+1][i+1];
		}
		I = TECDAT100(&III, x, &DIsDouble);
		DAVecRestoreArray(user[bi].da, user[bi].P, &p);

		/*
		DAVecGetArray(user[bi].da, user[bi].Nvert, &nvert);
		for (k=zs; k<ze-2; k++)
		for (j=ys; j<ye-2; j++)
		for (i=xs; i<xe-2; i++) {
			x[k * (mx-2)*(my-2) + j*(mx-2) + i] = nvert[k+1][j+1][i+1];
		}
		I = TECDAT100(&III, x, &DIsDouble);
		DAVecRestoreArray(user[bi].da, user[bi].Nvert, &nvert);
		*/

		PetscFree(x);
	}
	I = TECEND100();

	return 0;
}
#else
PetscErrorCode TECIOOutQ(UserCtx *user, int Q)
{
	PetscPrintf(PETSC_COMM_WORLD, "Compiled without Tecplot. Function TECIOOutQ not available!\n");
	return -1;
}
#endif

PetscErrorCode FormMetrics(UserCtx *user)
{
	DA		cda;
	Cmpnts	***csi, ***eta, ***zet;
	PetscScalar	***aj;
	Vec		coords;
	Cmpnts	***coor;

	DA		da = user->da, fda = user->fda;
	Vec		Csi = user->Csi, Eta = user->Eta, Zet = user->Zet;
	Vec		Aj = user->Aj;
	Vec		ICsi = user->ICsi, IEta = user->IEta, IZet = user->IZet;
	Vec		JCsi = user->JCsi, JEta = user->JEta, JZet = user->JZet;
	Vec		KCsi = user->KCsi, KEta = user->KEta, KZet = user->KZet;
	Vec		IAj = user->IAj, JAj = user->JAj, KAj = user->KAj;


	Cmpnts	***icsi, ***ieta, ***izet;
	Cmpnts	***jcsi, ***jeta, ***jzet;
	Cmpnts	***kcsi, ***keta, ***kzet;
	Cmpnts	***gs;
	PetscReal	***iaj, ***jaj, ***kaj;

	Vec		Cent = user->Cent; //local working array for storing cell center geometry

	Vec		Centx, Centy, Centz, lCoor;
	Cmpnts	***cent, ***centx, ***centy, ***centz;

	PetscInt	xs, ys, zs, xe, ye, ze;
	DALocalInfo	info;

	PetscInt	mx, my, mz;
	PetscInt	lxs, lxe, lys, lye, lzs, lze;

	PetscScalar	dxdc, dydc, dzdc, dxde, dyde, dzde, dxdz, dydz, dzdz;
	PetscInt	i, j, k, ia, ja, ka, ib, jb, kb;
	PetscInt	gxs, gxe, gys, gye, gzs, gze;
	PetscErrorCode	ierr;

	PetscReal	xcp, ycp, zcp, xcm, ycm, zcm;

	printf("Dathi here startsr\n");
	DAGetLocalInfo(da, &info);

	printf("Dathi ends startsr\n");
	mx = info.mx; my = info.my; mz = info.mz;
	xs = info.xs; xe = xs + info.xm;
	ys = info.ys; ye = ys + info.ym;
	zs = info.zs; ze = zs + info.zm;

	gxs = info.gxs; gxe = gxs + info.gxm;
	gys = info.gys; gye = gys + info.gym;
	gzs = info.gzs; gze = gzs + info.gzm;

	//printf("Dathi here startsr\n");
	DAGetCoordinateDA(da, &cda);
	DAVecGetArray(cda, Csi, &csi);
	DAVecGetArray(cda, Eta, &eta);
	DAVecGetArray(cda, Zet, &zet);
	ierr = DAVecGetArray(da, Aj,  &aj); CHKERRQ(ierr);
	

	printf("Dathi here ends\n");
	
	
	//DAVecGetArray(fda, Csi, &csi);
	//DAVecGetArray(fda, Eta, &eta);
	//DAVecGetArray(fda, Zet, &zet);
	//ierr = DAVecGetArray(da, Aj,  &aj); CHKERRQ(ierr);

	DAGetGhostedCoordinates(da, &coords);
	DAVecGetArray(fda, coords, &coor);


	//  VecDuplicate(coords, &Cent);
	lxs = xs; lxe = xe;
	lys = ys; lye = ye;
	lzs = zs; lze = ze;

	if (xs==0) lxs = xs+1;
	if (ys==0) lys = ys+1;
	if (zs==0) lzs = zs+1;

	if (xe==mx) lxe = xe-1;
	if (ye==my) lye = ye-1;
	if (ze==mz) lze = ze-1;

	/* Calculating transformation metrics in i direction */
	for (k=lzs; k<lze; k++) {
		for (j=lys; j<lye; j++) {
			for (i=xs; i<lxe; i++) {
				/* csi = de X dz */
				dxde = 0.5 * (coor[k  ][j  ][i  ].x + coor[k-1][j  ][i  ].x -
											coor[k  ][j-1][i  ].x - coor[k-1][j-1][i  ].x);
				dyde = 0.5 * (coor[k  ][j  ][i  ].y + coor[k-1][j  ][i  ].y -
											coor[k  ][j-1][i  ].y - coor[k-1][j-1][i  ].y);
				dzde = 0.5 * (coor[k  ][j  ][i  ].z + coor[k-1][j  ][i  ].z -
											coor[k  ][j-1][i  ].z - coor[k-1][j-1][i  ].z);

				dxdz = 0.5 * (coor[k  ][j-1][i  ].x + coor[k  ][j  ][i  ].x -
											coor[k-1][j-1][i  ].x - coor[k-1][j  ][i  ].x);
				dydz = 0.5 * (coor[k  ][j-1][i  ].y + coor[k  ][j  ][i  ].y -
											coor[k-1][j-1][i  ].y - coor[k-1][j  ][i  ].y);
				dzdz = 0.5 * (coor[k  ][j-1][i  ].z + coor[k  ][j  ][i  ].z -
											coor[k-1][j-1][i  ].z - coor[k-1][j  ][i  ].z);

				csi[k][j][i].x = dyde * dzdz - dzde * dydz;
				csi[k][j][i].y =-dxde * dzdz + dzde * dxdz;
				csi[k][j][i].z = dxde * dydz - dyde * dxdz;

			}
		}
	}

	// Need more work -- lg65
	/* calculating j direction metrics */
	for (k=lzs; k<lze; k++){
		for (j=ys; j<lye; j++){
			for (i=lxs; i<lxe; i++){

				/* eta = dz X de */
				dxdc = 0.5 * (coor[k  ][j  ][i  ].x + coor[k-1][j  ][i  ].x -
											coor[k  ][j  ][i-1].x - coor[k-1][j  ][i-1].x);
				dydc = 0.5 * (coor[k  ][j  ][i  ].y + coor[k-1][j  ][i  ].y -
											coor[k  ][j  ][i-1].y - coor[k-1][j  ][i-1].y);
				dzdc = 0.5 * (coor[k  ][j  ][i  ].z + coor[k-1][j  ][i  ].z -
											coor[k  ][j  ][i-1].z - coor[k-1][j  ][i-1].z);

				dxdz = 0.5 * (coor[k  ][j  ][i  ].x + coor[k  ][j  ][i-1].x -
											coor[k-1][j  ][i  ].x - coor[k-1][j  ][i-1].x);
				dydz = 0.5 * (coor[k  ][j  ][i  ].y + coor[k  ][j  ][i-1].y -
											coor[k-1][j  ][i  ].y - coor[k-1][j  ][i-1].y);
				dzdz = 0.5 * (coor[k  ][j  ][i  ].z + coor[k  ][j  ][i-1].z -
											coor[k-1][j  ][i  ].z - coor[k-1][j  ][i-1].z);

				eta[k][j][i].x = dydz * dzdc - dzdz * dydc;
				eta[k][j][i].y =-dxdz * dzdc + dzdz * dxdc;
				eta[k][j][i].z = dxdz * dydc - dydz * dxdc;

			}
		}
	}

	/* calculating k direction metrics */
	for (k=zs; k<lze; k++){
		for (j=lys; j<lye; j++){
			for (i=lxs; i<lxe; i++){
				dxdc = 0.5 * (coor[k  ][j  ][i  ].x + coor[k  ][j-1][i  ].x -
											coor[k  ][j  ][i-1].x - coor[k  ][j-1][i-1].x);
				dydc = 0.5 * (coor[k  ][j  ][i  ].y + coor[k  ][j-1][i  ].y -
											coor[k  ][j  ][i-1].y - coor[k  ][j-1][i-1].y);
				dzdc = 0.5 * (coor[k  ][j  ][i  ].z + coor[k  ][j-1][i  ].z -
											coor[k  ][j  ][i-1].z - coor[k  ][j-1][i-1].z);

				dxde = 0.5 * (coor[k  ][j  ][i  ].x + coor[k  ][j  ][i-1].x -
											coor[k  ][j-1][i  ].x - coor[k  ][j-1][i-1].x);
				dyde = 0.5 * (coor[k  ][j  ][i  ].y + coor[k  ][j  ][i-1].y -
											coor[k  ][j-1][i  ].y - coor[k  ][j-1][i-1].y);
				dzde = 0.5 * (coor[k  ][j  ][i  ].z + coor[k  ][j  ][i-1].z -
											coor[k  ][j-1][i  ].z - coor[k  ][j-1][i-1].z);

				zet[k][j][i].x = dydc * dzde - dzdc * dyde;
				zet[k][j][i].y =-dxdc * dzde + dzdc * dxde;
				zet[k][j][i].z = dxdc * dyde - dydc * dxde;

				/*	if ((i==1 || i==mx-2) && j==1 && (k==1 || k==0)) {
					PetscPrintf(PETSC_COMM_WORLD, "%e %e %e\n", dxdc * dyde, dydc * dxde, dzdc);
					PetscPrintf(PETSC_COMM_WORLD, "%e %e %e\n", dxde, dyde, dzde);
					PetscPrintf(PETSC_COMM_WORLD, "Met %e %e %e\n", zet[k][j][i].x, zet[k][j][i].y, zet[k][j][i].z);
					}*/

			}
		}
	}

	/* calculating Jacobian of the transformation */
	for (k=lzs; k<lze; k++){
		for (j=lys; j<lye; j++){
			for (i=lxs; i<lxe; i++){

				if (i>0 && j>0 && k>0) {
					dxdc = 0.25 * (coor[k  ][j  ][i  ].x + coor[k  ][j-1][i  ].x +
												 coor[k-1][j  ][i  ].x + coor[k-1][j-1][i  ].x -
												 coor[k  ][j  ][i-1].x - coor[k  ][j-1][i-1].x -
												 coor[k-1][j  ][i-1].x - coor[k-1][j-1][i-1].x);
					dydc = 0.25 * (coor[k  ][j  ][i  ].y + coor[k  ][j-1][i  ].y +
												 coor[k-1][j  ][i  ].y + coor[k-1][j-1][i  ].y -
												 coor[k  ][j  ][i-1].y - coor[k  ][j-1][i-1].y -
												 coor[k-1][j  ][i-1].y - coor[k-1][j-1][i-1].y);
					dzdc = 0.25 * (coor[k  ][j  ][i  ].z + coor[k  ][j-1][i  ].z +
												 coor[k-1][j  ][i  ].z + coor[k-1][j-1][i  ].z -
												 coor[k  ][j  ][i-1].z - coor[k  ][j-1][i-1].z -
												 coor[k-1][j  ][i-1].z - coor[k-1][j-1][i-1].z);

					dxde = 0.25 * (coor[k  ][j  ][i  ].x + coor[k  ][j  ][i-1].x +
												 coor[k-1][j  ][i  ].x + coor[k-1][j  ][i-1].x -
												 coor[k  ][j-1][i  ].x - coor[k  ][j-1][i-1].x -
												 coor[k-1][j-1][i  ].x - coor[k-1][j-1][i-1].x);
					dyde = 0.25 * (coor[k  ][j  ][i  ].y + coor[k  ][j  ][i-1].y +
												 coor[k-1][j  ][i  ].y + coor[k-1][j  ][i-1].y -
												 coor[k  ][j-1][i  ].y - coor[k  ][j-1][i-1].y -
												 coor[k-1][j-1][i  ].y - coor[k-1][j-1][i-1].y);
					dzde = 0.25 * (coor[k  ][j  ][i  ].z + coor[k  ][j  ][i-1].z +
												 coor[k-1][j  ][i  ].z + coor[k-1][j  ][i-1].z -
												 coor[k  ][j-1][i  ].z - coor[k  ][j-1][i-1].z -
												 coor[k-1][j-1][i  ].z - coor[k-1][j-1][i-1].z);

					dxdz = 0.25 * (coor[k  ][j  ][i  ].x + coor[k  ][j-1][i  ].x +
												 coor[k  ][j  ][i-1].x + coor[k  ][j-1][i-1].x -
												 coor[k-1][j  ][i  ].x - coor[k-1][j-1][i  ].x -
												 coor[k-1][j  ][i-1].x - coor[k-1][j-1][i-1].x);
					dydz = 0.25 * (coor[k  ][j  ][i  ].y + coor[k  ][j-1][i  ].y +
												 coor[k  ][j  ][i-1].y + coor[k  ][j-1][i-1].y -
												 coor[k-1][j  ][i  ].y - coor[k-1][j-1][i  ].y -
												 coor[k-1][j  ][i-1].y - coor[k-1][j-1][i-1].y);
					dzdz = 0.25 * (coor[k  ][j  ][i  ].z + coor[k  ][j-1][i  ].z +
												 coor[k  ][j  ][i-1].z + coor[k  ][j-1][i-1].z -
												 coor[k-1][j  ][i  ].z - coor[k-1][j-1][i  ].z -
												 coor[k-1][j  ][i-1].z - coor[k-1][j-1][i-1].z);

					aj[k][j][i] = dxdc * (dyde * dzdz - dzde * dydz) -
												dydc * (dxde * dzdz - dzde * dxdz) +
												dzdc * (dxde * dydz - dyde * dxdz);
					aj[k][j][i] = 1./aj[k][j][i];

					#ifdef NEWMETRIC
					csi[k][j][i].x = dyde * dzdz - dzde * dydz;
					csi[k][j][i].y =-dxde * dzdz + dzde * dxdz;
					csi[k][j][i].z = dxde * dydz - dyde * dxdz;

					eta[k][j][i].x = dydz * dzdc - dzdz * dydc;
					eta[k][j][i].y =-dxdz * dzdc + dzdz * dxdc;
					eta[k][j][i].z = dxdz * dydc - dydz * dxdc;

					zet[k][j][i].x = dydc * dzde - dzdc * dyde;
					zet[k][j][i].y =-dxdc * dzde + dzdc * dxde;
					zet[k][j][i].z = dxdc * dyde - dydc * dxde;
					#endif
				}
			}
		}
	}

	// mirror grid outside the boundary
	if (xs==0) {
		i = xs;
		for (k=zs; k<ze; k++)
		for (j=ys; j<ye; j++) {
			#ifdef NEWMETRIC
			csi[k][j][i] = csi[k][j][i+1];
			#endif
			eta[k][j][i] = eta[k][j][i+1];
			zet[k][j][i] = zet[k][j][i+1];
			aj[k][j][i] = aj[k][j][i+1];
		}
	}

	if (xe==mx) {
		i = xe-1;
		for (k=zs; k<ze; k++)
		for (j=ys; j<ye; j++) {
			#ifdef NEWMETRIC
			csi[k][j][i] = csi[k][j][i-1];
			#endif
			eta[k][j][i] = eta[k][j][i-1];
			zet[k][j][i] = zet[k][j][i-1];
			aj[k][j][i] = aj[k][j][i-1];
		}
	}


	if (ys==0) {
		j = ys;
		for (k=zs; k<ze; k++)
		for (i=xs; i<xe; i++) {
			#ifdef NEWMETRIC
			eta[k][j][i] = eta[k][j+1][i];
			#endif
			csi[k][j][i] = csi[k][j+1][i];
			zet[k][j][i] = zet[k][j+1][i];
			aj[k][j][i] = aj[k][j+1][i];
		}
	}


	if (ye==my) {
		j = ye-1;
		for (k=zs; k<ze; k++)
		for (i=xs; i<xe; i++) {
			#ifdef NEWMETRIC
			eta[k][j][i] = eta[k][j-1][i];
			#endif
			csi[k][j][i] = csi[k][j-1][i];
			zet[k][j][i] = zet[k][j-1][i];
			aj[k][j][i] = aj[k][j-1][i];
		}
	}


	if (zs==0) {
		k = zs;
		for (j=ys; j<ye; j++)
		for (i=xs; i<xe; i++) {
			#ifdef NEWMETRIC
			zet[k][j][i] = zet[k+1][j][i];
			#endif
			eta[k][j][i] = eta[k+1][j][i];
			csi[k][j][i] = csi[k+1][j][i];
			aj[k][j][i] = aj[k+1][j][i];
		}
	}


	if (ze==mz) {
		k = ze-1;
		for (j=ys; j<ye; j++)
		for (i=xs; i<xe; i++) {
			#ifdef NEWMETRIC
			zet[k][j][i] = zet[k-1][j][i];
			#endif
			eta[k][j][i] = eta[k-1][j][i];
			csi[k][j][i] = csi[k-1][j][i];
			aj[k][j][i] = aj[k-1][j][i];
		}
	}


	//  PetscPrintf(PETSC_COMM_WORLD, "Local info: %d", info.mx);



	DAVecRestoreArray(cda, Csi, &csi);
	DAVecRestoreArray(cda, Eta, &eta);
	DAVecRestoreArray(cda, Zet, &zet);
	DAVecRestoreArray(da, Aj,  &aj);


	DAVecRestoreArray(cda, coords, &coor);


	VecAssemblyBegin(Csi);
	VecAssemblyEnd(Csi);
	VecAssemblyBegin(Eta);
	VecAssemblyEnd(Eta);
	VecAssemblyBegin(Zet);
	VecAssemblyEnd(Zet);
	VecAssemblyBegin(Aj);
	VecAssemblyEnd(Aj);

	PetscBarrier(PETSC_NULL);
	return 0;
}

PetscErrorCode Ucont_P_Binary_Input(UserCtx *user)
{
	PetscViewer	viewer;

	char filen2[128];

	PetscOptionsClearValue("-vecload_block_size");
	sprintf(filen2, "pfield%05d_%1d.dat", ti, user->_this);

	PetscViewer	pviewer;
	//Vec temp;
	PetscInt rank;
	PetscReal norm;

	if (file_exist(filen2))
	if (!onlyV) {
		//DACreateNaturalVector(user->da, &temp);
	PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen2, FILE_MODE_READ, &pviewer);
	VecLoadIntoVector(pviewer, (user->P));
	VecNorm(user->P, NORM_INFINITY, &norm);
	PetscPrintf(PETSC_COMM_WORLD, "PIn %le\n", norm);
	PetscViewerDestroy(pviewer);
	//VecDestroy(temp);
	}

	if (nv_once) sprintf(filen2, "nvfield%05d_%1d.dat", 0, user->_this);
	else sprintf(filen2, "nvfield%05d_%1d.dat", ti, user->_this);

	if (cs) sprintf(filen2, "cs_%05d_%1d.dat", ti, user->_this);

	if ( !nv_once || (nv_once && ti==tis) ) {
	PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen2, FILE_MODE_READ, &pviewer);
	VecLoadIntoVector(pviewer, (user->Nvert));
	PetscViewerDestroy(pviewer);
	}

}

PetscErrorCode Ucont_P_Binary_Input1(UserCtx *user)
{
	PetscViewer viewer;
	char filen[128];

	sprintf(filen, "ufield%05d_%1d.dat", ti, user->_this);
	PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);

	PetscInt N;

	VecGetSize(user->Ucat, &N);
	PetscPrintf(PETSC_COMM_WORLD, "PPP %d\n", N);
	VecLoadIntoVector(viewer, (user->Ucat));
	PetscViewerDestroy(viewer);

	PetscBarrier(PETSC_NULL);
}

PetscErrorCode Ucont_P_Binary_Input_Averaging(UserCtx *user)
{
	PetscViewer viewer;
	char filen[128];
	/*
	sprintf(filen, "su0_%05d_%1d.dat", ti, user->_this);
	PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
	VecLoadIntoVector(viewer, (user->Ucat_sum));
	PetscViewerDestroy(viewer);

	sprintf(filen, "su1_%05d_%1d.dat", ti, user->_this);
	PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
	VecLoadIntoVector(viewer, (user->Ucat_cross_sum));
	PetscViewerDestroy(viewer);

	sprintf(filen, "su2_%05d_%1d.dat", ti, user->_this);
	PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
	VecLoadIntoVector(viewer, (user->Ucat_square_sum));
	PetscViewerDestroy(viewer);
	*/
	/*
	sprintf(filen, "sp_%05d_%1d.dat", ti, user->_this);
	PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
	VecLoadIntoVector(viewer, (user->P));
	PetscViewerDestroy(viewer);
	*/

	if (pcr) {
		Vec Ptmp;
		VecDuplicate(user->P, &Ptmp);

		sprintf(filen, "pfield%05d_%1d.dat", ti, user->_this);
		PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
		VecLoadIntoVector(viewer, user->P);
		PetscViewerDestroy(viewer);

		sprintf(filen, "sp_%05d_%1d.dat", ti, user->_this);
		PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
		VecLoadIntoVector(viewer, Ptmp);
		PetscViewerDestroy(viewer);


		VecScale(Ptmp, -1./((double)tis+1.0));
		VecAXPY(user->P, 1., Ptmp);

		VecDestroy(Ptmp);
	}

	if (nv_once) sprintf(filen, "nvfield%05d_%1d.dat", 0, user->_this);
	else sprintf(filen, "nvfield%05d_%1d.dat", ti, user->_this);

	//if (cs) sprintf(filen2, "cs_%05d_%1d.dat", ti, user->_this);

	if ( !nv_once || (nv_once && ti==tis) ) {
		PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
		VecLoadIntoVector(viewer, (user->Nvert));
		PetscViewerDestroy(viewer);
	}
	/*
	if ( !nv_once || (nv_once && ti==tis) ) {
		sprintf(filen, "nvfield%05d_%1d.dat", ti, user->_this);
		PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
		VecLoadIntoVector(viewer, (user->Nvert));
		PetscViewerDestroy(viewer);
	}
	*/
	PetscBarrier(PETSC_NULL);
}

PetscErrorCode Step_Injection(UserCtx *user, PetscInt ti, PetscInt tis, PetscInt tsteps)
{
	PetscInt    i, j, k, bi, numbytes, particle, NUM_OF_PARTICLE=1;
	Cmpnts      ***ucat, ***gcoor, ***pcoor, ***pvel;

	/*PetscPrintf(PETSC_COMM_WORLD," Dathi is heere! ..\n");*/

	Cmpnts      ***csi, ***eta, ***zet;

        Vec 	    P; //Pressure
	Vec         Csi = user->Csi, Eta = user->Eta, Zet = user->Zet;
	PetscReal   ***pr; // stored
	Vec         Coor, pCoor;
	FILE        *f, *fblock;
	char        filen[80], filen2[80];
	PetscInt    rank;
	size_t      offset = 0;
//	DA          cda;
	
	//DMDACoor3d  ***csi,***eta,***zet;
	//DM          cda;


	/*PetscPrintf(PETSC_COMM_WORLD," Dathi is heere! ..\n");*/

	PetscReal   *xpini, *ypini, *zpini, shortest;
	PetscReal   *xploc, *yploc, *zploc;
	PetscReal   *xpite, *ypite, *zpite;
	PetscReal   *pressure_pnew;
	PetscReal   *presuure_xpite, *presuure_ypite, *presuure_zpite;
	PetscReal   *uploc, *vploc, *wploc;
	PetscReal   *presuure_ploc_x, *presuure_ploc_y, *presuure_ploc_z;
	PetscReal   *presuure_ploc;
	PetscReal   ***dist;
	PetscInt    *ip, *jp, *kp;
	PetscReal   *dcutoff;
	//PetscReal   *c,*e,*z;

		
        
       	PetscReal   *dpdcsi, *dpdeta, *dpdzet;
	//Cmpnts	    ***dpdcsi, ***dpdeta, ***dpdzet;
	PetscReal    *dpdx, *dpdy, *dpdz, *delp;
	PetscReal   *Fpx, *Fpy, *Fpz,*Fp, Fb_x,Fb_y,Fb_z,FD_x,FD_y,FD_z,FA_x,FA_y,FA_z;
	PetscReal   rho;

	PetscInt    speedscale, starting_k;
	
	// Velocity scale = 3.937 m/s
	// Time scale = 6.452e-3 s
	// Dimensionless timestep = 0.024
	//     ===> Real timestep = 1.548e-4 s
	PetscReal   realtsteps=+1.548e-4, nondtsteps=realtsteps;

	PetscReal   dx, dy, dz, dxmin, dymin, dzmin;
	dxmin = +1.0e6; dymin = dxmin; dzmin = dxmin;

	PetscPrintf(PETSC_COMM_WORLD, "Initialize Particle Tracing Algorithm\n");
	PetscPrintf(PETSC_COMM_WORLD, "Current timestamp   #%i \n", ti);
	PetscPrintf(PETSC_COMM_WORLD, "Initial timestamp   #%i \n", tis);
	PetscPrintf(PETSC_COMM_WORLD, "Timeskip each frame = %i \n", tsteps);
	
	sprintf(filen2, "PARTICLE_%5.5d.txt", ti);// changed name to particle from msptf
	fblock = fopen(filen2, "w");

	PetscFPrintf(PETSC_COMM_WORLD, fblock, "xploc,yploc,zploc,uploc,vploc,wploc,pressure,PG_x,PG_y,PG_z,PG_Force_x,PG_Force_y,PG_Force_z,Buoyancy_Force\n");//added pressure to print

	for (bi=0; bi<block_number; bi++) {
		DA          da    = user[bi].da;
		DA          fda   = user[bi].fda;
	//	DA          cda   = user[bi].cda;
		DALocalInfo info  = user[bi].info;

		PetscInt    xs = info.xs, xe = info.xs + info.xm;
		PetscInt    ys = info.ys, ye = info.ys + info.ym;
		PetscInt    zs = info.zs, ze = info.zs + info.zm;
		PetscInt    mx = info.mx, my = info.my, mz = info.mz;

		i_begin = 1, i_end = mx-1;
		j_begin = 1, j_end = my-1;
		k_begin = 1, k_end = mz-1;

		xs = i_begin - 1, xe = i_end+1;
		ys = j_begin - 1, ye = j_end+1;
		zs = k_begin - 1, ze = k_end+1;

		DAGetCoordinates(da, &Coor);
		DAVecGetArray(fda, Coor, &gcoor);
		DAVecGetArray(fda, user[bi].Ucat, &ucat);
		DAVecGetArray(da, user[bi].P, &pr); //pressure array


		DAVecGetArray(user[bi].fda,user[bi].Csi, &csi);
		DAVecGetArray(user[bi].fda, user[bi].Eta, &eta);
		DAVecGetArray(user[bi].fda, user[bi].Zet, &zet);



		//PetscPrintf(PETSC_COMM_WORLD, "Dathi \n");


		PetscInt    lxs, lxe, lys, lye, lzs, lze;
		lxs = xs; lxe = xe; lys = ys; lye = ye; lzs = zs; lze = ze;

		if (lxs==0) lxs++;
		if (lxe==mx) lxe--;
		if (lys==0) lys++;
		if (lye==my) lye--;
		if (lzs==0) lzs++;
		if (lze==mz) lze--;

		speedscale      = user[bi].speedscale;
		starting_k      = user[bi].starting_k;
		nondtsteps      = user[bi].dt; // Extract nondimensional timestep
		NUM_OF_PARTICLE = user[bi].num_of_particle;
		//diameter_particle = user[bi].dp;
		//viscosity_fluid = user[bi].vis;
		//density_fluid = user[bi].rho;
		//density_particle = user[bi].rho_p;
		//gravity = user[bi].g;
		//velocity_fluid = user[bi].Vf;
		PetscReal v_scale = 5.5;
		PetscReal density_particle = 1602;
		PetscReal diameter_particle = 5e-6;
		PetscReal density_fluid = 1.168;
		//PetscReal mass_particle = 6.7104e-9;
		PetscReal mass_particle;
		PetscReal T_physical = 3.694e-4;
		PetscReal length_scale = 10.16e-2;
		PetscReal viscosity_fluid = 1.85e-5;



		PetscPrintf(PETSC_COMM_WORLD, "Timestep = %e \n", nondtsteps);
		PetscPrintf(PETSC_COMM_WORLD, "Number of Particle = %i\n", NUM_OF_PARTICLE);
		//DAVecGetArray(da, user[bi].Dist, &dist);

		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &xpini);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &ypini);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &zpini);
		
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &xploc);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &yploc);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &zploc);
		
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &xpite);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &ypite);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &zpite);

		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &pressure_pnew);

		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &presuure_xpite);
                PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &presuure_ypite);
                PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &presuure_zpite);


		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &uploc);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &vploc);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &wploc);

		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &presuure_ploc_x);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &presuure_ploc_y);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &presuure_ploc_z);

		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &presuure_ploc);

		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscInt), &ip);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscInt), &jp);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscInt), &kp);

		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &dcutoff);

		
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &dpdcsi);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &dpdeta);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &dpdzet);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &dpdx);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &dpdy);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &dpdz);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &delp);


		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &Fpx);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &Fpy);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &Fpz);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &Fp);

		//PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &Fb);




		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].xpini));
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].ypini));
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].zpini));
		
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].xploc));
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].yploc));
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].zploc));
		
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].xpite));
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].ypite));
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].zpite));

		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].pressure_pnew));


		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].presuure_xpite));
                PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].presuure_ypite));
                PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].presuure_zpite));

		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].uploc));
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].vploc));
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].wploc));

		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].presuure_ploc_x));
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].presuure_ploc_y));
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].presuure_ploc_z));

		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].presuure_ploc));

		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscInt), &(user[bi].ip));
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscInt), &(user[bi].jp));
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscInt), &(user[bi].kp));

		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].dcutoff));


		//PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].dpdcsi));
		//PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].dpdeta));
		//PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].dpdzet));
		//PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].dpdx));
		//PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].dpdy));
		//PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].dpdz));
		//PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].delp));


		//PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].Fpx));
		//PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].Fpy));
		//PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].Fpz));
		//PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].Fp));

		//PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &(user[bi].Fb));

		// Calculate delta x minimum
		for (i=lxs+1; i<lxe; i++) {
			k=0; j=0;
			//k = floor(0.5*(lzs+lze));
			//j = floor(0.5*(lys+lye));
			dx = sqrt( (gcoor[k][j][i].x - gcoor[k][j][i-1].x)*(gcoor[k][j][i].x - gcoor[k][j][i-1].x) + (gcoor[k][j][i].y - gcoor[k][j][i-1].y)*(gcoor[k][j][i].y - gcoor[k][j][i-1].y) + (gcoor[k][j][i].z - gcoor[k][j][i-1].z)*(gcoor[k][j][i].z - gcoor[k][j][i-1].z) );

			if (dxmin > dx) dxmin = dx;

		} // end of search for dxmin

		// Calculate delta y minimum
		for (j=lys+1; j<lye; j++) {
			k=0; i=0;
			//k = floor(0.5*(lzs+lze));
			//i = floor(0.5*(lxs+lxe));
			dy = sqrt( (gcoor[k][j][i].x - gcoor[k][j-1][i].x)*(gcoor[k][j][i].x - gcoor[k][j-1][i].x) + (gcoor[k][j][i].y - gcoor[k][j-1][i].y)*(gcoor[k][j][i].y - gcoor[k][j-1][i].y) + (gcoor[k][j][i].z - gcoor[k][j-1][i].z)*(gcoor[k][j][i].z - gcoor[k][j-1][i].z) );

			if (dymin > dy) dymin = dy;
		} // end of search for dymin

		// Calculate delta z minimum
		for (k=lzs+1; k<lze; k++) {
			j=0; i=0;
			//j = floor(0.5*(lys+lye));
			//i = floor(0.5*(lxs+lxe));
			dz = sqrt( (gcoor[k][j][i].x - gcoor[k-1][j][i].x)*(gcoor[k][j][i].x - gcoor[k-1][j][i].x) + (gcoor[k][j][i].y - gcoor[k-1][j][i].y)*(gcoor[k][j][i].y - gcoor[k-1][j][i].y) + (gcoor[k][j][i].z - gcoor[k-1][j][i].z)*(gcoor[k][j][i].z - gcoor[k-1][j][i].z) );

			if (dzmin > dz) dzmin = dz;
		} // end of search for dzmin
		PetscPrintf(PETSC_COMM_WORLD, "Delta x minimum = %e\n", dxmin);
		PetscPrintf(PETSC_COMM_WORLD, "Delta y minimum = %e\n", dymin);
		PetscPrintf(PETSC_COMM_WORLD, "Delta z minimum = %e\n", dzmin);

		user[bi].dxmin = dxmin;
		user[bi].dymin = dymin;
		user[bi].dzmin = dzmin;

		// THIS IS AN INJECTION STEP
		PetscPrintf(PETSC_COMM_WORLD, "===== Starting injection step =====");
		for (particle=0; particle<NUM_OF_PARTICLE; particle++) {
			// Injection position
			// Create random distribution
			//PetscInt    min=0, max=10;
			
			PetscInt    min=0, max=3;

			PetscInt    kpini=starting_k, jpini, ipini;

			srand(particle+kpini);

			//srand(particle+jpini);
			//srand(particle+ipini);
			//PetscInt  lp=0.0,up=1000.0;
                        
			
			PetscInt  lp=0.0,up=10;
			PetscReal random_x = (rand() % (up - lp + 1) + lp)*0.05;
                        PetscReal random_y = (rand() % (up - lp + 1) + lp)*0.05;
                        //PetscReal random_z = (rand() % (up - lp + 1) + lp)*0.000001;
			
			/*
			PetscReal r = 0.01; // Radius of the circle
    			PetscReal theta = ((double)rand() / RAND_MAX) * 2 * 3.14; // Random angle in radians

    			PetscReal random_x = r * cos(theta);
    			PetscReal random_y = r * sin(theta);
    			PetscReal random_z = ((double)rand() / RAND_MAX) * 0.01; // Random z value in [0, 0.01]
			*/

                        //PetscReal random_x = (rand() % (up - lp + 1) + lp)*0.0005;
                        //PetscReal random_y = (rand() % (up - lp + 1) + lp)*0.0005;
			//PetscReal random_z = (rand() % (up - lp + 1) + lp)*0.00001;


                        //PetscPrintf(PETSC_COMM_WORLD, "rand_vel_x = %.2f\n", random_vel_x);
                        //PetscPrintf(PETSC_COMM_WORLD, "rand_vel_y = %.2f\n", random_vel_y);
                        //PetscPrintf(PETSC_COMM_WORLD, "rand_vel_z = %.2f\n", random_vel_z);

			PetscReal   epsilon_x = rand() % (max - min + 1) + min;
			PetscReal   epsilon_y = rand() % (max - min + 1) + min;

			epsilon_x = epsilon_x/max; if (epsilon_x==0 || epsilon_x==1) epsilon_x = 0.5;
			epsilon_y = epsilon_y/max; if (epsilon_y==0 || epsilon_y==1) epsilon_y = 0.5;

			//PetscPrintf(PETSC_COMM_WORLD, "epsilon for x = %.2f\n", epsilon_x);
			//PetscPrintf(PETSC_COMM_WORLD, "epsilon for y = %.2f\n", epsilon_y);

			//ipini = floor(0.5*(lxs+lxe)) ;//floor(epsilon_x*(lxs+lxe));
			//jpini = floor(0.5*(lys+lye)) ;//floor(epsilon_y*(lys+lye)); 
			
			ipini = floor(epsilon_x*(lxs+lxe));
			jpini = floor(epsilon_y*(lys+lye)); 


			xpini[particle] = gcoor[kpini][jpini][ipini].x + random_x;
			xploc[particle] = xpini[particle];

			ypini[particle] = gcoor[kpini][jpini][ipini].y + random_y;
			yploc[particle] = ypini[particle];

			zpini[particle] = gcoor[kpini][jpini][ipini].z; //+ random_z;
			zploc[particle] = zpini[particle];


			// Particle velocity
			//uploc[particle] = ucat[kpini][jpini][ipini].x + random_vel_x;
			//vploc[particle] = ucat[kpini][jpini][ipini].y + random_vel_y;
			//wploc[particle] = ucat[kpini][jpini][ipini].z + random_vel_z;

			// Particle Pressure
			presuure_ploc_x[particle] = pr[kpini][jpini][ipini];

			// Pressure Gradient

			dpdcsi[particle]  = (pr[kpini][jpini][ipini+1] - pr[kpini][jpini][ipini-1]) / 2;
			dpdeta[particle]  = (pr[kpini][jpini+1][ipini] - pr[kpini][jpini-1][ipini]) / 2;
			dpdzet[particle]  = (pr[kpini+1][jpini][ipini] - pr[kpini-1][jpini][ipini]) / 2;
                       // printf("pr[kpini][jpini][ipini+1] = %e\n", dpdcsi[particle]);

			//double cs = csi[kpini][jpini][ipini];
        

			dpdx[particle] = dpdcsi[particle]*csi[kpini][jpini][ipini].x + dpdeta[particle]*eta[kpini][jpini][ipini].x + dpdzet[particle]*zet[kpini][jpini][ipini].x;

			dpdy[particle] = dpdcsi[particle]*csi[kpini][jpini][ipini].y + dpdeta[particle]*eta[kpini][jpini][ipini].y + dpdzet[particle]*zet[kpini][jpini][ipini].y;

			dpdz[particle] = dpdcsi[particle]*csi[kpini][jpini][ipini].z + dpdeta[particle]*eta[kpini][jpini][ipini].z + dpdzet[particle]*zet[kpini][jpini][ipini].z;

			//delp[particle] = sqrt(pow(dpdx[particle],2)+pow(dpdy[particle],2)+pow(dpdz[particle],2));


			// Pressure gradient force
			//pi= (3.141592625);

			//Fpx[particle] = (1/4)*pi*pow((diameter_particle),3)*dpdx[particle];
			//Fpy[particle] = (1/4)*pi*pow((diameter_particle),3)*dpdy[particle];
			//Fpz[particle] = (1/4)*pi*pow((diameter_particle),3)*dpdz[particle];

			//mass of the particle
			mass_particle = density_particle*(4/3)*3.14*pow((diameter_particle/2),3);

		        Fpx[particle] =  (0.25)*3.14*pow((diameter_particle),3)*dpdx[particle]*((density_fluid*pow(v_scale,2))/(length_scale));
		        Fpy[particle] =  (0.25)*3.14*pow((diameter_particle),3)*dpdy[particle]*((density_fluid*pow(v_scale,2))/(length_scale));
		        Fpz[particle] =  (0.25)*3.14*pow((diameter_particle),3)*dpdz[particle]*((density_fluid*pow(v_scale,2))/(length_scale));

		// Buoyancy Force
			Fb_x = 0;
			Fb_y = 0;
			Fb_z = 0;//(0.166)*3.14*pow((diameter_particle),3)*(density_particle - density_fluid)*9.81;
			
			
			PetscReal v_relative,v_relative_x, v_relative_y, v_relative_z;
			PetscReal fluid_velocity_x, fluid_velocity_y, fluid_velocity_z;
		
			fluid_velocity_x = ucat[kpini][jpini][ipini].x;		
			fluid_velocity_y = ucat[kpini][jpini][ipini].y;
			fluid_velocity_z = ucat[kpini][jpini][ipini].z;

		// Relative velocity = fluid - particle
			v_relative_x = fluid_velocity_x - uploc[particle];		
			v_relative_y = fluid_velocity_y - vploc[particle];
			v_relative_z = fluid_velocity_z - wploc[particle];
			v_relative = v_scale* (sqrt(v_relative_x* v_relative_x + v_relative_y* v_relative_y + v_relative_z * v_relative_z));

		//--------------------- TRung copy//
			PetscReal Re_s, CD, FD_x, FD_y, FD_z;
			PetscReal acceleration_x, acceleration_y, acceleration_z;
			Re_s  = (density_fluid*v_relative*density_particle)/viscosity_fluid;
			CD    = (24/Re_s)*(1 + (0.15*pow(Re_s,0.687)));

		// CHECK THE FORMULA FOR THE FORCES!!!
			FD_x =  CD*density_fluid*(3.14*(pow(diameter_particle,2))/8)*v_relative*(fluid_velocity_x - uploc[particle])*v_scale;
			FD_y =  CD*density_fluid*(3.14*(pow(diameter_particle,2))/8)*v_relative*(fluid_velocity_y - vploc[particle])*v_scale;//CHECK
			FD_z =  CD*density_fluid*(3.14*(pow(diameter_particle,2))/8)*v_relative*(fluid_velocity_z - wploc[particle])*v_scale;//CHECK
		//Added mass force
			FA_x = (-1/12)*3.14*pow(diameter_particle,3)*density_particle*((acceleration_x)*(pow(v_scale,2)/length_scale));
			FA_y = (-1/12)*3.14*pow(diameter_particle,3)*density_particle*((acceleration_y)*(pow(v_scale,2)/length_scale));
			FA_z = (-1/12)*3.14*pow(diameter_particle,3)*density_particle*((acceleration_z)*(pow(v_scale,2)/length_scale));
		// acceleration of particle
		// Acceleration should be in the non-dimensional form
			acceleration_x = ((Fpx[particle] + Fb_x +  FD_x +  FA_x)/mass_particle)*(length_scale/pow(v_scale,2));
			acceleration_y = ((Fpy[particle] + Fb_y +  FD_y +  FA_y)/mass_particle)*(length_scale/pow(v_scale,2));
			acceleration_z = ((Fpz[particle] + Fb_z +  FD_z +  FA_z)/mass_particle)*(length_scale/pow(v_scale,2));

		// Update the non-dimensional velocities
			uploc[particle] = 0;//uploc[particle] + acceleration_x*tsteps*nondtsteps;
			vploc[particle] = 0;//vploc[particle] + acceleration_y*tsteps*nondtsteps;
			wploc[particle] = 0;//wploc[particle] + acceleration_z*tsteps*nondtsteps; 
                        //printf("pi = %e\n", Fpx[particle]);
/*
			//Fp[particle]  = sqrt(pow(Fpx[particle],2) + pow(Fpy[particle],2) + pow(Fpz[particle],2));

			// Buoyancy Force

			//Fb[particle] = (1/6)*pi*pow((diameter_particle),3)*(density_particle - density_fluid)*gravity;

			//Fb[particle] = (0.166)*3.14*pow((0.001),3)*(2500 - density_fluid)*9.81;


                        //printf("pi = %d\n", density_fluid);

			//PetscPrintf(PETSC_COMM_WORLD, "pi  = %.2f\n", density_fluid);

			// Particle relative reynolds number

			//Vp[particle] = sqrt((uploc[particle],2) + (vploc[particle],2) + (wploc[particle],2));

			//Re_s[particle]  = (abs(Vp[particle] - velocity_fluid)*(diameter_particle))/viscosity_fluid;

			//Re_s[particle]  = ((velocity_fluid)*(diameter_particle))/viscosity_fluid;


			// Drag force coefficient
			
			//CD[particle] = (24/Re_s[particle])*(1+0.15*(pow(Re_s[particle],0.687)));

			// Drag force






			
			//PetscPrintf(PETSC_COMM_WORLD, "Dathi \n");
			
			// Find next location and check if its inside the pipe
 			
			PetscReal xpite_forcast, ypite_forcast, zpite_forcast;


			xpite_forcast = xploc[particle] + uploc[particle]*tsteps*nondtsteps*speedscale;
		        ypite_forcast = yploc[particle] + vploc[particle]*tsteps*nondtsteps*speedscale;
			zpite_forcast = zploc[particle] + wploc[particle]*tsteps*nondtsteps*speedscale;

			//centroid of the plane
			
			PetscReal X_centroid=0, Y_centroid=0, Z_centroid=0;
			PetscReal X_C = 0, Y_C = 0, Z_C = 0;

			for (j=lys; j<lye; j++) {
				for (i=lxs; i<lxe; i++) {

					X_C = X_C + gcoor[kpini][j  ][i  ].x;

					Y_C = Y_C + gcoor[kpini][j  ][i  ].y;

					Z_C = Z_C + gcoor[kpini][j  ][i  ].z;
					
									
				}
			}

			X_centroid = X_C/((lye-lys)*(lxe-lxs));
			Y_centroid = Y_C/((lye-lys)*(lxe-lxs));
			Z_centroid = Z_C/((lye-lys)*(lxe-lxs));

			// Projection
			PetscReal vector_x=0,vector_y=0,vector_z=0; 

			vector_x = xpite_forcast - X_centroid;

			vector_y = ypite_forcast - Y_centroid;
			
			vector_z = zpite_forcast - Z_centroid;
			


			//distance
			//PetscReal *normal_x, *normal_y, *normal_z, *distance, *Pp_x,*Pp_y,*Pp_z, *Op_x,*Op_y,*Op_z,radius_pipe=0.0508;


			PetscReal normal_x=0,normal_y=0,normal_z=0,distance=0,radius_pipe=0.0508;

			//normal_x  = zet[kpini][j][i].x/sqrt(pow(zet[kpini][j][i].x,2)+pow(zet[kpini][j][i].y,2)+pow(zet[kpini][j][i].z,2));

			//normal_y  = zet[kpini][j][i].y/sqrt(pow(zet[kpini][j][i].x,2)+pow(zet[kpini][j][i].y,2)+pow(zet[kpini][j][i].z,2));

			//normal_z  = zet[kpini][j][i].z/sqrt(pow(zet[kpini][j][i].x,2)+pow(zet[kpini][j][i].y,2)+pow(zet[kpini][j][i].z,2));


			normal_x  = zet[k][j][i].x/sqrt(pow(zet[k][j][i].x,2)+pow(zet[k][j][i].y,2)+pow(zet[k][j][i].z,2));

			normal_y  = zet[k][j][i].y/sqrt(pow(zet[k][j][i].x,2)+pow(zet[k][j][i].y,2)+pow(zet[k][j][i].z,2));

			normal_z  = zet[k][j][i].z/sqrt(pow(zet[k][j][i].x,2)+pow(zet[k][j][i].y,2)+pow(zet[k][j][i].z,2));

			distance = vector_x*normal_x + vector_y*normal_y + vector_z*normal_z;


			printf("dist=%le",distance);

			//printf("d = %le",distance);
			

			//Projected point
			PetscReal Pp_x,Pp_y,Pp_z,Op_x,Op_y,Op_z;
			//


			Pp_x = distance*normal_x;

			Pp_y = distance*normal_y;

			Pp_z = distance*normal_z;


			Op_x = vector_x - Pp_x;

			Op_y = vector_y - Pp_y;

			Op_z = vector_z - Pp_z;

			float radial_distance = sqrt(Op_x*Op_x + Op_y*Op_y + Op_z*Op_z);


			//radial_distance = sqrt(Op_x[particle]*Op_x[particle] + Op_y[particle]*Op_y[particle] + Op_z[particle]*Op_z[particle]);


			//printf("kpini=%d, X_centroid = %le, Y_centroid= %le, Z_centroid= %le, lye= %d, lxe =%d ", kpini,X_centroid,Y_centroid,Z_centroid,lye,lxe);
			
			printf("radial_distance = %le",radial_distance);


			if(radial_distance<radius_pipe){

						xpite[particle] = xploc[particle] + uploc[particle]*tsteps*nondtsteps*speedscale;
						ypite[particle] = yploc[particle] + vploc[particle]*tsteps*nondtsteps*speedscale;
						zpite[particle] = zploc[particle] + wploc[particle]*tsteps*nondtsteps*speedscale;
			}

			else {

						xpite[particle] = xploc[particle];  
						ypite[particle] = yploc[particle];
						zpite[particle] = zploc[particle];
			}


*/ 
			xpite[particle] = xploc[particle] + uploc[particle]*tsteps*nondtsteps*speedscale;
		        ypite[particle] = yploc[particle] + vploc[particle]*tsteps*nondtsteps*speedscale;
			zpite[particle] = zploc[particle] + wploc[particle]*tsteps*nondtsteps*speedscale;



			// pressure at next location
			
	        	/*pressure_pnew[particle] = presuure_ploc_x[particle]*tsteps*nondtsteps*speedscale;*/
			
			pressure_pnew[particle] = presuure_ploc_x[particle];

			printf("something\n");
		


			// THIS IS BY FAR ENOUGH FOR FIRST INJECTION STEP

			// Saving calculated values into global UserCtx
			user[bi].xploc[particle]   = xploc[particle];
			user[bi].yploc[particle]   = yploc[particle];
			user[bi].zploc[particle]   = zploc[particle];

			user[bi].uploc[particle]   = uploc[particle];
			user[bi].vploc[particle]   = vploc[particle];
			user[bi].wploc[particle]   = wploc[particle];

			user[bi].presuure_ploc_x[particle]   = presuure_ploc_x[particle];//


			user[bi].xpite[particle]   = xpite[particle];
			user[bi].ypite[particle]   = ypite[particle];
			user[bi].zpite[particle]   = zpite[particle];

			user[bi].pressure_pnew[particle]   = pressure_pnew[particle]; //

			user[bi].ip[particle]      = ipini;
			user[bi].jp[particle]      = jpini;
			user[bi].kp[particle]      = kpini;

			user[bi].dcutoff[particle] = dcutoff[particle];
			
			//user[bi].dpdcsi[particle]  = dpdcsi[particle];
			//user[bi].dpdeta[particle]  = dpdeta[particle];
			//user[bi].dpdzet[particle]  = dpdzet[particle];
			//user[bi].dpdx[particle]  = dpdx[particle];
			//user[bi].dpdy[particle]  = dpdy[particle];
			//user[bi].dpdz[particle]  = dpdz[particle];
			//user[bi].delp[particle]  = delp[particle];

			//user[bi].Fpx[particle]  = Fpx[particle];
			//user[bi].Fpy[particle]  = Fpy[particle];
			//user[bi].Fpz[particle]  = Fpz[particle];

			//user[bi].Fb[particle]  = Fb[particle];

                        PetscPrintf(PETSC_COMM_WORLD, "after DAT\n");

			PetscPrintf(PETSC_COMM_WORLD, "THIS IS AN INJECTION STEP\n");
			PetscPrintf(PETSC_COMM_WORLD, "Particle is injected at location  x y z = %e %e %e\n", xpini[particle], ypini[particle], zpini[particle]);
			PetscPrintf(PETSC_COMM_WORLD, "Particle's velocity is founded as u v w = %e %e %e\n", uploc[particle], vploc[particle], zploc[particle]);
			PetscPrintf(PETSC_COMM_WORLD, "Particle will be located next at  x y z = %e %e %e\n", xpite[particle], ypite[particle], zpite[particle]);

			PetscPrintf(PETSC_COMM_WORLD, "TRACING DONE FOR PARTICLE #%i\n", particle);

			PetscFPrintf(PETSC_COMM_WORLD, fblock, "%e,%e,%e,%e,%e,%e,%e,%e,%e,%e,%e,%e,%e\n", xploc[particle], yploc[particle], zploc[particle], uploc[particle], vploc[particle], wploc[particle], presuure_ploc_x[particle], dpdx[particle], dpdy[particle], dpdz[particle], Fpx[particle], Fpy[particle], Fpz[particle]);


		//	PetscPrintf(PETSC_COMM_WORLD," Dathi is heere! ..\n");

		} //End of particle loop
		DAVecRestoreArray(fda, Coor, &gcoor);
		DAVecRestoreArray(fda, user[bi].Ucat, &ucat);
		//DAVecRestoreArray(fda, user[bi].P, &pr);
		DAVecRestoreArray(da, user[bi].P, &pr);
		//DAGetCoordinateDA(da, &cda);
		DAVecRestoreArray(fda, Csi, &csi);
		DAVecRestoreArray(fda, Eta, &eta);
		DAVecRestoreArray(fda, Zet, &zet);
		
	} // End of for loop

	PetscPrintf(PETSC_COMM_WORLD," Dathi is heere! ..\n");


	PetscFree(xpini); PetscFree(ypini); PetscFree(zpini);
	PetscFree(xploc); PetscFree(yploc); PetscFree(zploc);
	PetscFree(xpite); PetscFree(ypite); PetscFree(zpite);
	PetscFree(uploc); PetscFree(vploc); PetscFree(wploc);
	PetscFree(ip);    PetscFree(jp);    PetscFree(kp);
	PetscFree(dcutoff);

	fclose(fblock);

	return(0);

}

PetscErrorCode Step_TimeIteration(UserCtx *user, PetscInt ti, PetscInt tis, PetscInt tsteps)
{
	PetscInt    i, j, k, bi, numbytes, particle, NUM_OF_PARTICLE=1;
	Cmpnts      ***ucat, ***gcoor, ***pcoor, ***pvel;
	Cmpnts      ***csi, ***eta, ***zet;
	Vec         Coor, pCoor;
	Vec         Csi = user->Csi, Eta = user->Eta, Zet = user->Zet;
	Vec 	    P;
	PetscReal   ***pr;
	FILE        *f, *fblock;
	char        filen[80], filen2[80];
	PetscInt    rank, fast=0, interpolation=0;
	size_t      offset = 0;

	PetscReal   *xpini, *ypini, *zpini,*ppini, shortest;
	PetscReal   *xploc, *yploc, *zploc, *pploc;
	PetscReal   *xpite, *ypite, *zpite;
	PetscReal   *uploc, *vploc, *wploc;
	PetscReal   ***dist;
	PetscInt    *ip, *jp, *kp;
	PetscInt    *npi, *npj, *npk;
	PetscReal   *dcutoff;
	PetscReal   *presuure_ploc_x;
	PetscReal   *pressure_pnew;
	PetscReal   *dpdcsi, *dpdeta, *dpdzet;
	PetscReal    *dpdx, *dpdy, *dpdz, *delp;
	PetscReal   *Fpx, *Fpy, *Fpz,*Fp, Fb_x,Fb_y,Fb_z, FA_x, FA_y, FA_z;

	// Fluid properties
	PetscReal v_scale = 5.5;
	PetscReal density_particle = 1602;
	PetscReal diameter_particle = 10e-6;
	PetscReal density_fluid = 1.168;
	//PetscReal mass_particle = 6.7104e-9;
	PetscReal mass_particle;
	PetscReal T_physical = 3.694e-4;
	PetscReal length_scale = 10.16e-2;
	PetscReal viscosity_fluid = 1.85e-5;
       	PetscReal uploc_temp, vploc_temp, wploc_temp;
	PetscReal Reflection;


	PetscInt    speedscale;
	// Velocity scale = 3.937 m/s
	// Time scale = 6.452e-3 s
	// Dimensionless timestep = 0.024
	//     ===> Real timestep = 1.548e-4 s
	PetscReal   realtsteps=+1.548e-4, nondtsteps=realtsteps;

	//PetscReal a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13;

	PetscPrintf(PETSC_COMM_WORLD, "Initialize Particle Tracing Algorithm\n");
	PetscPrintf(PETSC_COMM_WORLD, "Current timestamp   #%i \n", ti);
	PetscPrintf(PETSC_COMM_WORLD, "Initial timestamp   #%i \n", tis);
	PetscPrintf(PETSC_COMM_WORLD, "Timeskip each frame = %i \n", tsteps);

	//sprintf(filen2, "MSPTf_%5.5d.txt", ti);
	sprintf(filen2, "PARTICLE_%5.5d.txt", ti);
	fblock = fopen(filen2, "w");

	//PetscFPrintf(PETSC_COMM_WORLD, fblock, "xploc,yploc,zploc,uploc,vploc,wploc\n");
	//PetscFPrintf(PETSC_COMM_WORLD, fblock, "xploc,yploc,zploc,uploc,vploc,wploc,pressure\n");

	PetscFPrintf(PETSC_COMM_WORLD, fblock, "xploc,yploc,zploc,uploc,vploc,wploc,pressure,PG_x,PG_y,PG_z,PG_Force_x,PG_Force_y,PG_Force_z\n");//added pressure to print



	for (bi=0; bi<block_number; bi++) {
		DA          da    = user[bi].da;
		DA          fda   = user[bi].fda;
		DALocalInfo info  = user[bi].info;

		PetscInt    xs = info.xs, xe = info.xs + info.xm;
		PetscInt    ys = info.ys, ye = info.ys + info.ym;
		PetscInt    zs = info.zs, ze = info.zs + info.zm;
		PetscInt    mx = info.mx, my = info.my, mz = info.mz;

		i_begin = 1, i_end = mx-1;
		j_begin = 1, j_end = my-1;
		k_begin = 1, k_end = mz-1;

		xs = i_begin - 1, xe = i_end+1;
		ys = j_begin - 1, ye = j_end+1;
		zs = k_begin - 1, ze = k_end+1;

		DAGetCoordinates(da, &Coor);
		DAVecGetArray(fda, Coor, &gcoor);
		DAVecGetArray(fda, user[bi].Ucat, &ucat);
		DAVecGetArray(da, user[bi].P, &pr);


		DAVecGetArray(user[bi].fda,user[bi].Csi, &csi);
		DAVecGetArray(user[bi].fda, user[bi].Eta, &eta);
		DAVecGetArray(user[bi].fda, user[bi].Zet, &zet);

		PetscInt    lxs, lxe, lys, lye, lzs, lze;
		lxs = xs; lxe = xe; lys = ys; lye = ye; lzs = zs; lze = ze;

		if (lxs==0) lxs++;
		if (lxe==mx) lxe--;
		if (lys==0) lys++;
		if (lye==my) lye--;
		if (lzs==0) lzs++;
		if (lze==mz) lze--;

		PetscReal   dxmin, dymin, dzmin;
		dxmin           = user[bi].dxmin;
		dymin           = user[bi].dymin;
		dzmin           = user[bi].dzmin;
		speedscale      = user[bi].speedscale;
		nondtsteps      = user[bi].dt; // Extract nondimensional timestep
		NUM_OF_PARTICLE = user[bi].num_of_particle;

		

		//PetscPrintf(PETSC_COMM_WORLD, "Timestep = %e \n", nondtsteps);
		DAVecGetArray(da, user[bi].Dist, &dist);

		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &dcutoff);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &xpini);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &ypini);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &zpini);

		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &xploc);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &yploc);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &zploc);

		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &xpite);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &ypite);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &zpite);

		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &uploc);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &vploc);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &wploc);

		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &presuure_ploc_x);


		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &dpdcsi);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &dpdeta);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &dpdzet);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &dpdx);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &dpdy);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &dpdz);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &delp);


		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &Fpx);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &Fpy);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &Fpz);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &Fp);



		
		//PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscReal), &Fb);


		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscInt), &ip);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscInt), &jp);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscInt), &kp);

		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscInt), &npi);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscInt), &npj);
		PetscMalloc(NUM_OF_PARTICLE*sizeof(PetscInt), &npk);
		//   Below is the calculation of dxmin, dymin, dzmin at the second timestamp
		// THIS IS NOT AN INJECTION STEP
		PetscPrintf(PETSC_COMM_WORLD," Dathi is heere! ..\n");

		for (particle=0; particle<NUM_OF_PARTICLE; particle++) {
			ip[particle]    = user[bi].ip[particle];
			jp[particle]    = user[bi].jp[particle];
			kp[particle]    = user[bi].kp[particle];

			xpini[particle] = user[bi].xploc[particle];
			ypini[particle] = user[bi].yploc[particle];
			zpini[particle] = user[bi].zploc[particle];

			xploc[particle] = user[bi].xpite[particle];
			yploc[particle] = user[bi].ypite[particle];
			zploc[particle] = user[bi].zpite[particle];
/*
			xploc[particle] = user[bi].xploc[particle];
			yploc[particle] = user[bi].yploc[particle];
			zploc[particle] = user[bi].zploc[particle];
			
			xpite[particle] = user[bi].xpite[particle];
			ypite[particle] = user[bi].ypite[particle];
			zpite[particle] = user[bi].zpite[particle];
			
*/			uploc[particle] = user[bi].uploc[particle];
			vploc[particle] = user[bi].vploc[particle];
			wploc[particle] = user[bi].wploc[particle];


			// 3rd-order Taylor Series "TS13" Scheme (Yeung and Pope, 1987)
			/*
			a1 = 0.25*( -xploc[particle] -yploc[particle] -zploc[particle] + 2*yploc[particle]*yploc[particle] + 2*xploc[particle]*yploc[particle] + 2*xploc[particle]*zploc[particle] + 2*yploc[particle]*zploc[particle] - 4*xploc[particle]*yploc[particle]*zploc[particle]  );

			a2 = 0.25*( -xploc[particle] -yploc[particle] +zploc[particle] + 2*yploc[particle]*yploc[particle] + 2*xploc[particle]*yploc[particle] - 2*xploc[particle]*zploc[particle] - 2*yploc[particle]*zploc[particle] + 4*xploc[particle]*yploc[particle]*zploc[particle]  );

			a3 = 0.25*( xploc[particle] -yploc[particle] +zploc[particle] + 2*yploc[particle]*yploc[particle] - 2*xploc[particle]*yploc[particle] - 2*xploc[particle]*zploc[particle] + 2*yploc[particle]*zploc[particle] - 4*xploc[particle]*yploc[particle]*zploc[particle]  );

			a4 = 0.25*( xploc[particle] -yploc[particle] -zploc[particle] + 2*yploc[particle]*yploc[particle] - 2*xploc[particle]*yploc[particle] - 2*xploc[particle]*zploc[particle] + 2*yploc[particle]*zploc[particle] + 4*xploc[particle]*yploc[particle]*zploc[particle]  );

			a5 = 0.25*( -xploc[particle] +yploc[particle] -zploc[particle] + 2*yploc[particle]*yploc[particle] - 2*xploc[particle]*yploc[particle] + 2*xploc[particle]*zploc[particle] - 2*yploc[particle]*zploc[particle] + 4*xploc[particle]*yploc[particle]*zploc[particle]  );

			a6 = 0.25*( -xploc[particle] +yploc[particle] +zploc[particle] + 2*yploc[particle]*yploc[particle] - 2*xploc[particle]*yploc[particle] - 2*xploc[particle]*zploc[particle] + 2*yploc[particle]*zploc[particle] - 4*xploc[particle]*yploc[particle]*zploc[particle]  );

			a7 = 0.25*( xploc[particle] +yploc[particle] +zploc[particle] + 2*yploc[particle]*yploc[particle] + 2*xploc[particle]*yploc[particle] + 2*xploc[particle]*zploc[particle] + 2*yploc[particle]*zploc[particle] + 4*xploc[particle]*yploc[particle]*zploc[particle]  );

			a8 = 0.25*( xploc[particle] +yploc[particle] -zploc[particle] + 2*yploc[particle]*yploc[particle] + 2*xploc[particle]*yploc[particle] - 2*xploc[particle]*zploc[particle] - 2*yploc[particle]*zploc[particle] - 4*xploc[particle]*yploc[particle]*zploc[particle]  );

			a9 = 1 - xploc[particle]*xploc[particle] - 2*yploc[particle]*yploc[particle] - zploc[particle]*zploc[particle];

			a10 = 0.5*( zploc[particle]*zploc[particle] - yploc[particle]*yploc[particle] );

			a11 = 0.5*( xploc[particle]*xploc[particle] - yploc[particle]*yploc[particle] );

			a12 = a10;

			a13 = a11;

			PetscPrintf(PETSC_COMM_WORLD, "a1 = %e\n", a1);
			PetscPrintf(PETSC_COMM_WORLD, "a2 = %e\n", a2);
			PetscPrintf(PETSC_COMM_WORLD, "a3 = %e\n", a3);
			PetscPrintf(PETSC_COMM_WORLD, "a4 = %e\n", a4);
			PetscPrintf(PETSC_COMM_WORLD, "a5 = %e\n", a5);
			PetscPrintf(PETSC_COMM_WORLD, "a6 = %e\n", a6);
			PetscPrintf(PETSC_COMM_WORLD, "a7 = %e\n", a7);
			PetscPrintf(PETSC_COMM_WORLD, "a8 = %e\n", a8);
			PetscPrintf(PETSC_COMM_WORLD, "a9 = %e\n", a9);
			PetscPrintf(PETSC_COMM_WORLD, "a10 = %e\n", a10);
			PetscPrintf(PETSC_COMM_WORLD, "a11 = %e\n", a11);
			PetscPrintf(PETSC_COMM_WORLD, "a12 = %e\n", a12);
			PetscPrintf(PETSC_COMM_WORLD, "a13 = %e\n", a13);
			*/

			PetscReal shortest=+1.0e6;
			//ip[particle] = 5;
			//jp[particle] = 5;
			//kp[particle] = 5;
			// Calculate particle velocity by locating closest grid point
			// Find ip, jp, kp
			//   2 approaches
			//     1. Brute force (fast==0)
			//     2. Using cutoff diameter and steepest descent
			if (!fast) { // Using Brute Force
				for (k=lzs; k<lze; k++) {
					for (j=lys; j<lye; j++) {
						for (i=lxs; i<lxe; i++) {
							dist[k][j][i] = sqrt( (gcoor[k][j][i].x - xploc[particle])*(gcoor[k][j][i].x - xploc[particle]) + (gcoor[k][j][i].y - yploc[particle])*(gcoor[k][j][i].y - yploc[particle]) + (gcoor[k][j][i].z - zploc[particle])*(gcoor[k][j][i].z - zploc[particle]) );

							if (shortest > dist[k][j][i]) {
								shortest = dist[k][j][i];

								ip[particle] = i;
								jp[particle] = j;
								kp[particle] = k;
							} // ip, jp, kp found
						}
					 }
				} // End of Brute Force search
			} // End of if
			else { // Using Diameter Cutoff
				dcutoff[particle] = 4*sqrt( (xploc[particle] - xpini[particle])*(xploc[particle] - xpini[particle]) + (yploc[particle] - ypini[particle])*(yploc[particle] - ypini[particle]) + (zploc[particle] - zpini[particle])*(zploc[particle] - zpini[particle]) );

				PetscPrintf(PETSC_COMM_WORLD, "cutoff for particle %i is %e\n", particle, dcutoff[particle]);

				npi[particle] = floor( dcutoff[particle]/dxmin ); // Current length scale = 1 inch
				if (npi[particle]==0 || npi[particle]==1) npi[particle]++;

				npj[particle] = floor( dcutoff[particle]/dymin );
				if (npj[particle]==0 || npj[particle]==1) npj[particle]++;

				npk[particle] = floor( dcutoff[particle]/dzmin );
				if (npk[particle]==0 || npk[particle]==1) npk[particle]++;

				// Search for particle within cutoff range
				lzs = user[bi].kp[particle]-npk[particle];
				lze = user[bi].kp[particle]+npk[particle];

				PetscPrintf(PETSC_COMM_WORLD, "%i %i %i %i %i\n", npi[particle], npj[particle], npk[particle], lzs, lze);
				//====================================================
				//
				for (k=lzs; k<lze; k++) {
					for (j=lys; j<lye; j++) {
						for (i=lxs; i<lxe; i++) {

								ip[particle] = -1;
								jp[particle] = -1;
								kp[particle] = -1;
							} // ip, jp, kp reset 
						}
					}
	
				//
				//
				//
				//
				//
				//
				//
				//
			    shortest = 1.0e+6;	
				for (k=lzs; k<lze; k++) {
					for (j=lys; j<lye; j++) {
						for (i=lxs; i<lxe; i++) {
					//for (j=user[bi].jp[particle]-npj[particle]; j<user[bi].jp[particle]+npj[particle]; j++) {
						//for (i=user[bi].ip[particle]-npi[particle]; i<user[bi].ip[particle]+npi[particle]; i++) {
							// First implementation, no steepest descent
							dist[k][j][i] = sqrt( (gcoor[k][j][i].x - xploc[particle])*(gcoor[k][j][i].x - xploc[particle]) + (gcoor[k][j][i].y - yploc[particle])*(gcoor[k][j][i].y - yploc[particle]) + (gcoor[k][j][i].z - zploc[particle])*(gcoor[k][j][i].z - zploc[particle]) );

							if (shortest > dist[k][j][i]) {
								shortest = dist[k][j][i];

								ip[particle] = i;
								jp[particle] = j;
								kp[particle] = k;
							} // ip, jp, kp found
						}
					}
				}
			}
			if (!interpolation) {
				PetscReal random_vel_x;
				PetscReal random_vel_y;
				PetscReal random_vel_z;


				//uploc[particle] = ucat[kp[particle]][jp[particle]][ip[particle]].x + random_vel_x;
				//vploc[particle] = ucat[kp[particle]][jp[particle]][ip[particle]].y + random_vel_y;
				//wploc[particle] = ucat[kp[particle]][jp[particle]][ip[particle]].z + random_vel_z;

				presuure_ploc_x[particle] = pr[kp[particle]][jp[particle]][ip[particle]];


				dpdcsi[particle]  = (pr[kp[particle]][jp[particle]][ip[particle]+1]- pr[kp[particle]][jp[particle]][ip[particle] -1]) / 2;
				dpdeta[particle]  = (pr[kp[particle]][jp[particle]+1][ip[particle]] - pr[kp[particle]][jp[particle]-1][ip[particle]]) / 2;
				dpdzet[particle]  = (pr[kp[particle]+1][jp[particle]][ip[particle]] - pr[kp[particle]-1][jp[particle]][ip[particle]]) / 2;

				
				dpdx[particle] = dpdcsi[particle]*csi[kp[particle]][jp[particle]][ip[particle]].x + dpdeta[particle]*eta[kp[particle]][jp[particle]][ip[particle]].x + dpdzet[particle]*zet[kp[particle]][jp[particle]][ip[particle]].x;

				dpdy[particle] = dpdcsi[particle]*csi[kp[particle]][jp[particle]][ip[particle]].y + dpdeta[particle]*eta[kp[particle]][jp[particle]][ip[particle]].y + dpdzet[particle]*zet[kp[particle]][jp[particle]][ip[particle]].y;

				dpdz[particle] = dpdcsi[particle]*csi[kp[particle]][jp[particle]][ip[particle]].z + dpdeta[particle]*eta[kp[particle]][jp[particle]][ip[particle]].z + dpdzet[particle]*zet[kp[particle]][jp[particle]][ip[particle]].z;

				//mass of the particle
				mass_particle = density_particle*(4/3)*3.14*pow((diameter_particle/2),3);
			
		     
				//The equation for the pressure gradient??
				Fpx[particle] =  (0.25)*3.14*pow((diameter_particle),3)*dpdx[particle]*((density_fluid*pow(v_scale,2))/(length_scale));
				Fpy[particle] =  (0.25)*3.14*pow((diameter_particle),3)*dpdy[particle]*((density_fluid*pow(v_scale,2))/(length_scale));
				Fpz[particle] =  (0.25)*3.14*pow((diameter_particle),3)*dpdz[particle]*((density_fluid*pow(v_scale,2))/(length_scale));

				// Buoyancy Force
				Fb_x = 0;
				Fb_y = 0;
				Fb_z = ((0.166)*3.14*pow((diameter_particle),3)*(density_particle - density_fluid)*9.81)*(pow(length_scale,2)*density_fluid*pow(v_scale,2));


				PetscReal v_relative,v_relative_x, v_relative_y, v_relative_z;
				PetscReal fluid_velocity_x, fluid_velocity_y, fluid_velocity_z;
		
				fluid_velocity_x = ucat[kp[particle]][jp[particle]][ip[particle]].x;		
				fluid_velocity_y = ucat[kp[particle]][jp[particle]][ip[particle]].y;
				fluid_velocity_z = ucat[kp[particle]][jp[particle]][ip[particle]].z;

				// Relative velocity = fluid - particle
				v_relative_x = fluid_velocity_x - uploc[particle];		
				v_relative_y = fluid_velocity_y - vploc[particle];
				v_relative_z = fluid_velocity_z - wploc[particle];
				v_relative = v_scale* (sqrt(v_relative_x* v_relative_x + v_relative_y* v_relative_y + v_relative_z * v_relative_z));

				//--------------------- TRung copy//
				PetscReal Re_s, CD, FD_x, FD_y, FD_z,Force_normal_x,Force_normal_y,Force_normal_z,Force_normal_eta_N,Force_normal_csi_zero_x,Force_normal_csi_zero_y,Force_normal_csi_zero_z,Force_normal_csi_N_x,Force_normal_csi_N_y,Force_normal_csi_N_z,Force_normal_eta_zero_x,Force_normal_eta_zero_y,Force_normal_eta_zero_z,Force_normal_eta_N_x,Force_normal_eta_N_y,Force_normal_eta_N_z;
				PetscReal acceleration_x, acceleration_y, acceleration_z;
				Re_s  = (density_fluid*v_relative*density_particle)/viscosity_fluid;
				CD    = (24/Re_s)*(1 + (0.15*pow(Re_s,0.687)));

				// CHECK THE FORMULA FOR THE FORCES!!!
				FD_x =  CD*density_fluid*(3.14*(pow(diameter_particle,2))/8)*v_relative*(fluid_velocity_x - uploc[particle])*v_scale;
				FD_y =  CD*density_fluid*(3.14*(pow(diameter_particle,2))/8)*v_relative*(fluid_velocity_y - vploc[particle])*v_scale;//CHECK
				FD_z =  CD*density_fluid*(3.14*(pow(diameter_particle,2))/8)*v_relative*(fluid_velocity_z - wploc[particle])*v_scale;//CHECK

				FA_x = (-1/12)*3.14*pow(diameter_particle,3)*density_particle*((acceleration_x)*(pow(v_scale,2)/length_scale));
				FA_y = (-1/12)*3.14*pow(diameter_particle,3)*density_particle*((acceleration_y)*(pow(v_scale,2)/length_scale));
				FA_z = (-1/12)*3.14*pow(diameter_particle,3)*density_particle*((acceleration_z)*(pow(v_scale,2)/length_scale));

				// acceleration of particle
				PetscReal Force_normal, Force_normal_eta_zero,  Force_normal_csi_zero,Force_normal_csi_N;

				PetscReal acceleration_x_temp, acceleration_y_temp, acceleration_z_temp;

				acceleration_x_temp = ((Fpx[particle] + Fb_x +  FD_x + FA_x )/mass_particle)*(length_scale/pow(v_scale,2));

			        acceleration_y_temp = ((Fpy[particle] + Fb_y +  FD_y + FA_y )/mass_particle)*(length_scale/pow(v_scale,2));

				acceleration_z_temp = ((Fpz[particle] + Fb_z +  FD_z + FA_z )/mass_particle)*(length_scale/pow(v_scale,2));

						
				//=========== CHECK IF THE PARTICLE IS INSIDE ===========///
				// Update the non-dimensional velocities
				uploc_temp = uploc[particle] + acceleration_x_temp*tsteps*nondtsteps;
				vploc_temp = vploc[particle] + acceleration_y_temp*tsteps*nondtsteps;
				wploc_temp = wploc[particle] + acceleration_z_temp*tsteps*nondtsteps; 
				
				//uploc_temp = uploc_temp + acceleration_x_temp*tsteps*nondtsteps;
				//vploc_temp = vploc_temp + acceleration_y_temp*tsteps*nondtsteps;
				//wploc_temp = wploc_temp + acceleration_z_temp*tsteps*nondtsteps;
				


				// If the particle is close to the surface 
				Reflection =0; // Originally we do not need reflecction
				/*			 //
				if ( (ip[particle] = lxs) || (ip[particle] = lxe) || (jp[particle] = lys) || (jp[particle] = lye)){
				       	Reflection=1; // Needs reflection!
				} */

			//	if ( ((ip[particle]  < 2  ) || (ip[particle] >  mx -2)|| (jp[particle] < 2 ) ||(jp[particle] =  my-2)) && (radial_distance < ))
			//	{
			//	       	Reflection=1;

			//	PetscPrintf(PETSC_COMM_WORLD," Reflected .............particle = %d ip = %d\n", particle, ip[particle]);
			//	exit(0);	
			//	}



				//=========== CHECK IF THE PARTICLE IS INSIDE ===========///

		
				PetscReal xpite_forcast=0, ypite_forcast=0, zpite_forcast=0;

				xpite_forcast = xploc[particle] + uploc[particle]*nondtsteps;
				ypite_forcast = yploc[particle] + vploc[particle]*nondtsteps;
				zpite_forcast = zploc[particle] + wploc[particle]*nondtsteps;

				printf("x_F=%le\n",xpite_forcast);

				printf("y_F=%le\n",ypite_forcast);
				printf("z_F=%le\n",zpite_forcast);

				//centroid of the plane
		
				PetscReal X_centroid=0, Y_centroid=0, Z_centroid=0;
				PetscReal X_C = 0, Y_C = 0, Z_C = 0;
		
                        			for (j=lys; j<lye; j++) {
                                			for (i=lxs; i<lxe; i++) {

                                        			X_C = X_C + gcoor[kp[particle]][j][i].x;

                                        			Y_C = Y_C + gcoor[kp[particle]][j][i].y;

                                        			Z_C = Z_C + gcoor[kp[particle]][j][i].z;


                                			}
                        			}

                        			X_centroid = X_C/((lye-lys)*(lxe-lxs));
                        			Y_centroid = Y_C/((lye-lys)*(lxe-lxs));
                        			Z_centroid = Z_C/((lye-lys)*(lxe-lxs));


                        	// Projection
                        	PetscReal vector_x=0,vector_y=0,vector_z=0;
                        	
                        	vector_x  = xpite_forcast - X_centroid;
				vector_y  = ypite_forcast - Y_centroid;
				vector_z  = zpite_forcast - Z_centroid;

				PetscReal normal_x=0,normal_y=0,normal_z=0,distance=0,radius_pipe=2; 
				//PetscReal normal_xk, normal_yk, normal_zk , magnitude,wall_grid_distance= 5;
			        

                        	//normal_x  = zet[k][j][i].x/sqrt(pow(zet[k][j][i].x,2)+pow(zet[k][j][i].y,2)+pow(zet[k][j][i].z,2));
                        	//normal_y  = zet[k][j][i].y/sqrt(pow(zet[k][j][i].x,2)+pow(zet[k][j][i].y,2)+pow(zet[k][j][i].z,2));
                        	//normal_z  = zet[k][j][i].z/sqrt(pow(zet[k][j][i].x,2)+pow(zet[k][j][i].y,2)+pow(zet[k][j][i].z,2));

				normal_x  = zet[kp[particle]][jp[particle]][ip[particle]].x/sqrt(pow(zet[kp[particle]][jp[particle]][ip[particle]].x,2)+pow(zet[kp[particle]][jp[particle]][ip[particle]].y,2)+pow(zet[kp[particle]][jp[particle]][ip[particle]].z,2));
				normal_y  = zet[kp[particle]][jp[particle]][ip[particle]].y/sqrt(pow(zet[kp[particle]][jp[particle]][ip[particle]].x,2)+pow(zet[kp[particle]][jp[particle]][ip[particle]].y,2)+pow(zet[kp[particle]][jp[particle]][ip[particle]].z,2));
				normal_z  = zet[kp[particle]][jp[particle]][ip[particle]].z/sqrt(pow(zet[kp[particle]][jp[particle]][ip[particle]].x,2)+pow(zet[kp[particle]][jp[particle]][ip[particle]].y,2)+pow(zet[kp[particle]][jp[particle]][ip[particle]].z,2));



				distance = vector_x*normal_x + vector_y*normal_y + vector_z*normal_z;

                        	//printf("dist=%le",distance);

                    	

                        	//Projected point
                        	PetscReal Pp_x=0,Pp_y=0,Pp_z=0,Op_x=0,Op_y=0,Op_z=0;
                        

                        	Pp_x = distance*normal_x;
                        	Pp_y = distance*normal_y;
                       	 	Pp_z = distance*normal_z;

				Op_x = vector_x - Pp_x;
                        	Op_y = vector_y - Pp_y;
                        	Op_z = vector_z - Pp_z;

                        	float radial_distance = sqrt(Op_x*Op_x + Op_y*Op_y + Op_z*Op_z);

				printf("radial_distance = %le",radial_distance);
			
				Reflection = 0;

				//if  ((ip[particle] <= 3) || (ip[particle] >= 7) || (jp[particle] <= 3) ||(jp[particle] >= 7)) //&& (radial_distance >= 0.8*radius_pipe ))
				if  ((ip[particle] == 1) || (ip[particle] == 9) || (jp[particle] == 1) ||(jp[particle] == 9)) //&& (radial_distance >= 0.8*radius_pipe ))
				{
				       	Reflection= 1;

				PetscPrintf(PETSC_COMM_WORLD," Reflected .............particle = %d ip = %d\n", particle, ip[particle]);
				PetscPrintf(PETSC_COMM_WORLD," Reflected .............particle = %d jp = %d\n", particle, jp[particle]);
                                //exit(0);	
				}

				
				//PetscReal beta = 1.0e-10, a_e = 0.001, wall_grid_distance= 5, normal_xp, normal_yp, normal_zp, magnitude;
				PetscReal beta = 1.0e-8;
			        PetscReal a_e = 1.0e-4;
				PetscReal percentage_boundary= 0.95;
				PetscReal  wall_grid_distance= 5, normal_xp, normal_yp, normal_zp, magnitude,normal_curl_x,normal_curl_y,normal_curl_z;
				// If the particle goes outside of the pipe
				// Force the particle to sit still -> DO NOT MOVE
                        	//if(radial_distance >= 0.9* radius_pipe)
                        	/*
				if(radial_distance >=  percentage_boundary*radius_pipe && radial_distance < radius_pipe )
				{

				       if(ip[particle] < wall_grid_distance)
				       {
                        			normal_curl_x  =   eta[kp[particle]][jp[particle]][ip[particle]].y*zet[kp[particle]][jp[particle]][ip[particle]].z - eta[kp[particle]][jp[particle]][ip[particle]].z*zet[kp[particle]][jp[particle]][ip[particle]].y;
						normal_curl_y  = - eta[kp[particle]][jp[particle]][ip[particle]].x*zet[kp[particle]][jp[particle]][ip[particle]].z + eta[kp[particle]][jp[particle]][ip[particle]].z*zet[kp[particle]][jp[particle]][ip[particle]].x;
						normal_curl_z  =   eta[kp[particle]][jp[particle]][ip[particle]].x*zet[kp[particle]][jp[particle]][ip[particle]].y - eta[kp[particle]][jp[particle]][ip[particle]].y*zet[kp[particle]][jp[particle]][ip[particle]].x; 
 						magnitude      =   sqrt(pow(normal_curl_x,2) + pow(normal_curl_y,2) + pow(normal_curl_z,2));

						normal_xp = normal_curl_x/magnitude;
	                                        normal_yp = normal_curl_y/magnitude;
	                                        normal_zp = normal_curl_z/magnitude;

				       }

				      if(ip[particle] > mx - wall_grid_distance)
				      {
						normal_curl_x  =   eta[kp[particle]][jp[particle]][ip[particle]].y*zet[kp[particle]][jp[particle]][ip[particle]].z - eta[kp[particle]][jp[particle]][ip[particle]].z*zet[kp[particle]][jp[particle]][ip[particle]].y;
						normal_curl_y  = - eta[kp[particle]][jp[particle]][ip[particle]].x*zet[kp[particle]][jp[particle]][ip[particle]].z + eta[kp[particle]][jp[particle]][ip[particle]].z*zet[kp[particle]][jp[particle]][ip[particle]].x;
						normal_curl_z  =   eta[kp[particle]][jp[particle]][ip[particle]].x*zet[kp[particle]][jp[particle]][ip[particle]].y - eta[kp[particle]][jp[particle]][ip[particle]].y*zet[kp[particle]][jp[particle]][ip[particle]].x; 
 						magnitude      =   sqrt(pow(normal_curl_x,2) + pow(normal_curl_y,2) + pow(normal_curl_z,2));

						normal_xp = -normal_curl_x/magnitude;
	                                        normal_yp = -normal_curl_y/magnitude;
	                                        normal_zp = -normal_curl_z/magnitude;

				       }

				      
				      if(jp[particle]  <   wall_grid_distance)
				      {
						normal_curl_x  =   zet[kp[particle]][jp[particle]][ip[particle]].y*csi[kp[particle]][jp[particle]][ip[particle]].z - zet[kp[particle]][jp[particle]][ip[particle]].z*csi[kp[particle]][jp[particle]][ip[particle]].y;
						normal_curl_y  =   zet[kp[particle]][jp[particle]][ip[particle]].z*csi[kp[particle]][jp[particle]][ip[particle]].x - zet[kp[particle]][jp[particle]][ip[particle]].x*csi[kp[particle]][jp[particle]][ip[particle]].z;
						normal_curl_z  =   zet[kp[particle]][jp[particle]][ip[particle]].x*csi[kp[particle]][jp[particle]][ip[particle]].y - zet[kp[particle]][jp[particle]][ip[particle]].y*csi[kp[particle]][jp[particle]][ip[particle]].x; 
 						magnitude      =   sqrt(pow(normal_curl_x,2) + pow(normal_curl_y,2) + pow(normal_curl_z,2));

						normal_xp = normal_curl_x/magnitude;
	                                        normal_yp = normal_curl_y/magnitude;
	                                        normal_zp = normal_curl_z/magnitude;
												       
				       }
				      if(jp[particle] > my - wall_grid_distance)
				      {
						normal_curl_x  =   zet[kp[particle]][jp[particle]][ip[particle]].y*csi[kp[particle]][jp[particle]][ip[particle]].z - zet[kp[particle]][jp[particle]][ip[particle]].z*csi[kp[particle]][jp[particle]][ip[particle]].y;
						normal_curl_y  =   zet[kp[particle]][jp[particle]][ip[particle]].z*csi[kp[particle]][jp[particle]][ip[particle]].x - zet[kp[particle]][jp[particle]][ip[particle]].x*csi[kp[particle]][jp[particle]][ip[particle]].z;
						normal_curl_z  =   zet[kp[particle]][jp[particle]][ip[particle]].x*csi[kp[particle]][jp[particle]][ip[particle]].y - zet[kp[particle]][jp[particle]][ip[particle]].y*csi[kp[particle]][jp[particle]][ip[particle]].x; 
 						magnitude      =   sqrt(pow(normal_curl_x,2) + pow(normal_curl_y,2) + pow(normal_curl_z,2));

						normal_xp = -normal_curl_x/magnitude;
	                                        normal_yp = -normal_curl_y/magnitude;
	                                        normal_zp = -normal_curl_z/magnitude;
				      }
				PetscReal Reaction;
				//Reaction= a_e* pow((radial_distance - radius_pipe)/radius_pipe, -2);
				//PetscReal vel_mag = sqrt(uploc_temp * uploc_temp + vploc_temp * vploc_temp + wploc_temp * wploc_temp);

				//Reaction = mass_particle * vel_mag* fabs(radius_pipe/(radial_distance - radius_pipe));
 

				Force_normal_x =  beta*a_e*(normal_xp);  
				Force_normal_y =  beta*a_e*(normal_yp); 
				Force_normal_z =  beta*a_e*(normal_zp); 
				
				acceleration_x = ((Fpx[particle] + Fb_x +  FD_x + FA_x + Force_normal_x)/mass_particle)*(length_scale/pow(v_scale,2));
			        acceleration_y = ((Fpy[particle] + Fb_y +  FD_y + FA_y + Force_normal_y)/mass_particle)*(length_scale/pow(v_scale,2));
				acceleration_z = ((Fpz[particle] + Fb_z +  FD_z + FA_z + Force_normal_z)/mass_particle)*(length_scale/pow(v_scale,2));


				uploc[particle] = uploc[particle] + acceleration_x*tsteps*nondtsteps;
				vploc[particle] = vploc[particle] + acceleration_y*tsteps*nondtsteps;
				wploc[particle] = wploc[particle] + acceleration_z*tsteps*nondtsteps;

				}// End of outside of the pipe or in the inner part
				else
				{
				    if (radial_distance < percentage_boundary*radius_pipe) 
				    { 
				     	uploc[particle] = uploc_temp;
				        vploc[particle] = vploc_temp;
				    	wploc[particle] = wploc_temp;
				    }
				    
				    else // if it is on the surface of the pipe, stop it!
				    {
					uploc[particle] = 0;
					vploc[particle] = 0;
					wploc[particle] = 0;


				    } 				    
                                   
				}
			*/
				if (Reflection == 1)
				{
					
					// This needs checking!!
					//if(ip[particle] <= 3)
					if(ip[particle] == 1)
				       {
                        			normal_curl_x  =   eta[kp[particle]][jp[particle]][ip[particle]].y*zet[kp[particle]][jp[particle]][ip[particle]].z - eta[kp[particle]][jp[particle]][ip[particle]].z*zet[kp[particle]][jp[particle]][ip[particle]].y;
						normal_curl_y  = - eta[kp[particle]][jp[particle]][ip[particle]].x*zet[kp[particle]][jp[particle]][ip[particle]].z + eta[kp[particle]][jp[particle]][ip[particle]].z*zet[kp[particle]][jp[particle]][ip[particle]].x;
						normal_curl_z  =   eta[kp[particle]][jp[particle]][ip[particle]].x*zet[kp[particle]][jp[particle]][ip[particle]].y - eta[kp[particle]][jp[particle]][ip[particle]].y*zet[kp[particle]][jp[particle]][ip[particle]].x; 
 						magnitude      =   sqrt(pow(normal_curl_x,2) + pow(normal_curl_y,2) + pow(normal_curl_z,2));

						normal_xp = normal_curl_x/magnitude;
	                                        normal_yp = normal_curl_y/magnitude;
	                                        normal_zp = normal_curl_z/magnitude;

				       }
				       
				
				// This is correct----------  
				      //if(ip[particle] >= 7)
				      if(ip[particle] == 9)
				      {
						normal_curl_x  =   eta[kp[particle]][jp[particle]][ip[particle]].y*zet[kp[particle]][jp[particle]][ip[particle]].z - eta[kp[particle]][jp[particle]][ip[particle]].z*zet[kp[particle]][jp[particle]][ip[particle]].y;
						normal_curl_y  = - eta[kp[particle]][jp[particle]][ip[particle]].x*zet[kp[particle]][jp[particle]][ip[particle]].z + eta[kp[particle]][jp[particle]][ip[particle]].z*zet[kp[particle]][jp[particle]][ip[particle]].x;
						normal_curl_z  =   eta[kp[particle]][jp[particle]][ip[particle]].x*zet[kp[particle]][jp[particle]][ip[particle]].y - eta[kp[particle]][jp[particle]][ip[particle]].y*zet[kp[particle]][jp[particle]][ip[particle]].x; 
 						magnitude      =   sqrt(pow(normal_curl_x,2) + pow(normal_curl_y,2) + pow(normal_curl_z,2));

						normal_xp = -normal_curl_x/magnitude;
	                                        normal_yp = -normal_curl_y/magnitude;
	                                        normal_zp = -normal_curl_z/magnitude;

				       }
				
					//---------------------------correct---//
				
				      //if(jp[particle] <= 3)
				      if(jp[particle] == 1)
				      {
						normal_curl_x  =   zet[kp[particle]][jp[particle]][ip[particle]].y*csi[kp[particle]][jp[particle]][ip[particle]].z - zet[kp[particle]][jp[particle]][ip[particle]].z*csi[kp[particle]][jp[particle]][ip[particle]].y;
						normal_curl_y  =   zet[kp[particle]][jp[particle]][ip[particle]].z*csi[kp[particle]][jp[particle]][ip[particle]].x - zet[kp[particle]][jp[particle]][ip[particle]].x*csi[kp[particle]][jp[particle]][ip[particle]].z;
						normal_curl_z  =   zet[kp[particle]][jp[particle]][ip[particle]].x*csi[kp[particle]][jp[particle]][ip[particle]].y - zet[kp[particle]][jp[particle]][ip[particle]].y*csi[kp[particle]][jp[particle]][ip[particle]].x; 
 						magnitude      =   sqrt(pow(normal_curl_x,2) + pow(normal_curl_y,2) + pow(normal_curl_z,2));

						normal_xp = normal_curl_x/magnitude;
	                                        normal_yp = normal_curl_y/magnitude;
	                                        normal_zp = normal_curl_z/magnitude;
												       
				      }

				      //if(jp[particle] >= 7)
				      if(jp[particle] == 9)
				      {
						normal_curl_x  =   zet[kp[particle]][jp[particle]][ip[particle]].y*csi[kp[particle]][jp[particle]][ip[particle]].z - zet[kp[particle]][jp[particle]][ip[particle]].z*csi[kp[particle]][jp[particle]][ip[particle]].y;
						normal_curl_y  =   zet[kp[particle]][jp[particle]][ip[particle]].z*csi[kp[particle]][jp[particle]][ip[particle]].x - zet[kp[particle]][jp[particle]][ip[particle]].x*csi[kp[particle]][jp[particle]][ip[particle]].z;
						normal_curl_z  =   zet[kp[particle]][jp[particle]][ip[particle]].x*csi[kp[particle]][jp[particle]][ip[particle]].y - zet[kp[particle]][jp[particle]][ip[particle]].y*csi[kp[particle]][jp[particle]][ip[particle]].x; 
 						magnitude      =   sqrt(pow(normal_curl_x,2) + pow(normal_curl_y,2) + pow(normal_curl_z,2));

						normal_xp = -normal_curl_x/magnitude;
	                                        normal_yp = -normal_curl_y/magnitude;
	                                        normal_zp = -normal_curl_z/magnitude;
				      }


				      PetscReal vn_mag, vt_mag, vt_x, vt_y, vt_z, vn_new_x, vn_new_y, vn_new_z;

				      vn_mag = uploc_temp * normal_xp + vploc_temp * normal_yp + wploc_temp * normal_zp;
			/*	
				      vt_x = uploc_temp +  vn_mag * normal_xp;
				      vt_y = vploc_temp +  vn_mag * normal_yp;
				      vt_z = wploc_temp +  vn_mag * normal_zp;

				      vn_new_x = - vn_mag * normal_xp;
				      vn_new_y = - vn_mag * normal_yp;
				      vn_new_z = - vn_mag * normal_zp;
				      */
				      
				      vn_new_x = - vn_mag * normal_xp;
				      vn_new_y = - vn_mag * normal_yp;
				      vn_new_z = - vn_mag * normal_zp;
				      

				      vt_x = (0.5*uploc_temp +  vn_new_x);
				      vt_y = (0.5*vploc_temp +  vn_new_y);
				      vt_z = (0.5*wploc_temp +  vn_new_z);
				      
				      //vt_x = uploc_temp;
				      //vt_y = vploc_temp;
				      //vt_z = wploc_temp;

				      // Combine the components back
				      uploc[particle] = (vt_x + 0.7*vn_new_x);
				      vploc[particle] = (vt_y + 0.7*vn_new_y);
				      wploc[particle] = (vt_z + 0.7*vn_new_z);
				      
				      
				
				}//end of region close to the boundary
				else
				 {

				        //if (radial_distance < radius_pipe) 
					//{ 
				
					     uploc[particle] = uploc_temp;
					     vploc[particle] = vploc_temp;
					     wploc[particle] = wploc_temp;
					//}
					//else if (ip[particle]<1 || ip[particle]>9 || jp[particle]<1 || jp[particle]>9)
					//{
						//uploc[particle] = 0;
						//vploc[particle] = 0;
						//wploc[particle] = 0;
					//}
				   
				  }
				//============================================END OF CHECK ///

			}// Interpolation condition
			
			// Find next location
			xpite[particle] = xploc[particle] + uploc[particle]*tsteps*nondtsteps*speedscale;
			ypite[particle] = yploc[particle] + vploc[particle]*tsteps*nondtsteps*speedscale;
			zpite[particle] = zploc[particle] + wploc[particle]*tsteps*nondtsteps*speedscale;

			// Saving calculated values into global UserCtx
			user[bi].xploc[particle] = xploc[particle];
			user[bi].yploc[particle] = yploc[particle];
			user[bi].zploc[particle] = zploc[particle];

			user[bi].uploc[particle] = uploc[particle];
			user[bi].vploc[particle] = vploc[particle];
			user[bi].wploc[particle] = wploc[particle];

			user[bi].presuure_ploc_x[particle] = presuure_ploc_x[particle];

			user[bi].xpite[particle] = xpite[particle];
			user[bi].ypite[particle] = ypite[particle];
			user[bi].zpite[particle] = zpite[particle];

			user[bi].ip[particle]    = ip[particle];
			user[bi].jp[particle]    = jp[particle];
			user[bi].kp[particle]    = kp[particle];


			//user[bi].dpdcsi[particle]  = dpdcsi[particle];
			//user[bi].dpdeta[particle]  = dpdeta[particle];
			//user[bi].dpdzet[particle]  = dpdzet[particle];
			//user[bi].dpdx[particle]  = dpdx[particle];
			//user[bi].dpdy[particle]  = dpdy[particle];
			//user[bi].dpdz[particle]  = dpdz[particle];
			//user[bi].delp[particle]  = delp[particle];


			//user[bi].Fpx[particle]  = Fpx[particle];
			//user[bi].Fpy[particle]  = Fpy[particle];
			//user[bi].Fpz[particle]  = Fpz[particle];

			//user[bi].Fb[particle]  = Fb[particle];


			PetscPrintf(PETSC_COMM_WORLD, "THIS IS AN ITERATION STEP\n");
			PetscPrintf(PETSC_COMM_WORLD, "Particle is injected at location  x y z = %e %e %e\n", xploc[particle], yploc[particle], zploc[particle]);
			PetscPrintf(PETSC_COMM_WORLD, "Particle's velocity is founded as u v w = %e %e %e\n", uploc[particle], vploc[particle], zploc[particle]);
			PetscPrintf(PETSC_COMM_WORLD, "Particle will be located next at  x y z = %e %e %e\n", xpite[particle], ypite[particle], zpite[particle]);

			//PetscFPrintf(PETSC_COMM_WORLD, fblock, "%e,%e,%e,%e,%e,%e,%e\n", xploc[particle], yploc[particle], zploc[particle], uploc[particle], vploc[particle], wploc[particle],presuure_ploc_x[particle]);

			PetscFPrintf(PETSC_COMM_WORLD, fblock, "%e,%e,%e,%e,%e,%e,%e,%e,%e,%e,%e,%e,%e\n", xploc[particle], yploc[particle], zploc[particle], uploc[particle], vploc[particle], wploc[particle], presuure_ploc_x[particle], dpdx[particle], dpdy[particle], dpdz[particle], Fpx[particle], Fpy[particle], Fpz[particle]);


		} // End of particle loop

		DAVecRestoreArray(fda, Coor, &gcoor);
		DAVecRestoreArray(fda, user[bi].Ucat, &ucat);

		if (ti==tie) {
			PetscFree(user[bi].dcutoff);

			PetscFree(user[bi].xpini);
			PetscFree(user[bi].ypini);
			PetscFree(user[bi].zpini);

			PetscFree(user[bi].xploc);
			PetscFree(user[bi].yploc);
			PetscFree(user[bi].zploc);

			PetscFree(user[bi].xpite);
			PetscFree(user[bi].ypite);
			PetscFree(user[bi].zpite);

			PetscFree(user[bi].uploc);
			PetscFree(user[bi].vploc);
			PetscFree(user[bi].wploc);

			PetscFree(user[bi].presuure_ploc_x);
			PetscFree(user[bi].dpdx);
			PetscFree(user[bi].dpdy);
			PetscFree(user[bi].dpdz);
			PetscFree(user[bi].Fpx);
			PetscFree(user[bi].Fpy);
			PetscFree(user[bi].Fpz);
			

			PetscFree(user[bi].ip);
			PetscFree(user[bi].jp);
			PetscFree(user[bi].kp);
		}
	} // End of for loop

	PetscFree(xpini); PetscFree(ypini); PetscFree(zpini);
	PetscFree(xploc); PetscFree(yploc); PetscFree(zploc);
	PetscFree(xpite); PetscFree(ypite); PetscFree(zpite);
	PetscFree(uploc); PetscFree(vploc); PetscFree(wploc);
	PetscFree(ip);    PetscFree(jp);    PetscFree(kp);
	PetscFree(npi);   PetscFree(npj);   PetscFree(npk);
	PetscFree(dcutoff);

	PetscFree(presuure_ploc_x);

	fclose(fblock);

	return(0);

}

#undef __FUNCT__
#define __FUNCT__ "main"

int main(int argc, char **argv)
{
	PetscTruth flag;

	DA	da, fda;
	Vec	qn, qnm;
	Vec	c;
	UserCtx	*user;

	PetscErrorCode ierr;

	IBMNodes	ibm, *ibm0, *ibm1;

	PetscInitialize(&argc, &argv, (char *)0, help);

	PetscOptionsInsertFile(PETSC_COMM_WORLD, "control.dat", PETSC_TRUE);

	char tmp_str[256];
	PetscOptionsGetString(PETSC_NULL, "-prefix", tmp_str, 256, &flag);
	if (flag)sprintf(prefix, "%s_", tmp_str);
	else sprintf(prefix, "");

	PetscOptionsGetInt(PETSC_NULL, "-vc", &vc, PETSC_NULL);
	PetscOptionsGetInt(PETSC_NULL, "-binary", &binary_input, &flag);
	PetscOptionsGetInt(PETSC_NULL, "-xyz", &xyz_input, &flag);
	PetscOptionsGetInt(PETSC_NULL, "-rans", &rans, PETSC_NULL);
	PetscOptionsGetInt(PETSC_NULL, "-ransout", &rans_output, PETSC_NULL);
	PetscOptionsGetInt(PETSC_NULL, "-levelset", &levelset, PETSC_NULL);
	PetscOptionsGetInt(PETSC_NULL, "-avg", &avg, &flag);
	PetscOptionsGetInt(PETSC_NULL, "-shear", &shear, &flag);
	PetscOptionsGetInt(PETSC_NULL, "-averaging", &averaging_option, &flag);	// from control.dat

	PetscOptionsGetInt(PETSC_NULL, "-cs", &cs, &flag);
	PetscOptionsGetInt(PETSC_NULL, "-i_periodic", &i_periodic, &flag);
	PetscOptionsGetInt(PETSC_NULL, "-j_periodic", &j_periodic, &flag);
	PetscOptionsGetInt(PETSC_NULL, "-k_periodic", &k_periodic, &flag);

	PetscOptionsGetInt(PETSC_NULL, "-ii_periodic", &i_periodic, &flag);
	PetscOptionsGetInt(PETSC_NULL, "-jj_periodic", &j_periodic, &flag);
	PetscOptionsGetInt(PETSC_NULL, "-kk_periodic", &k_periodic, &flag);

	PetscOptionsGetInt(PETSC_NULL, "-nv", &nv_once, &flag);
	PetscOptionsGetInt(PETSC_NULL, "-vtk", &vtkOutput, &flag);
	printf("nv_once=%d\n", nv_once);

	int QCR = 0;
	PetscOptionsGetInt(PETSC_NULL, "-qcr", &QCR, PETSC_NULL);

	PetscOptionsGetInt(PETSC_NULL, "-tis", &tis, &flag);
	if (!flag) PetscPrintf(PETSC_COMM_WORLD, "Need the starting number!\n");

	PetscOptionsGetInt(PETSC_NULL, "-tie", &tie, &flag);
	if (!flag) tie = tis;

	PetscOptionsGetInt(PETSC_NULL, "-ts", &tsteps, &flag);
	if (!flag) tsteps = 5; /* Default increasement is 5 */

	PetscOptionsGetInt(PETSC_NULL, "-onlyV", &onlyV, &flag);
	PetscOptionsGetInt(PETSC_NULL, "-iavg", &i_average, &flag);
	PetscOptionsGetInt(PETSC_NULL, "-javg", &j_average, &flag);
	PetscOptionsGetInt(PETSC_NULL, "-kavg", &k_average, &flag);
	PetscOptionsGetInt(PETSC_NULL, "-ikavg", &ik_average, &flag);
	PetscOptionsGetInt(PETSC_NULL, "-pcr", &pcr, &flag);
	PetscOptionsGetInt(PETSC_NULL, "-reynolds", &reynolds, &flag);

	PetscOptionsGetInt(PETSC_NULL, "-ikcavg", &ikc_average, &flag);
	if (flag) {
		PetscTruth flag1, flag2;
		PetscOptionsGetInt(PETSC_NULL, "-pi", &pi, &flag1);
		PetscOptionsGetInt(PETSC_NULL, "-pk", &pk, &flag2);

		if (!flag1 || !flag2) {
			printf("To use -ikcavg you must set -pi and -pk, which are number of points in i- and k- directions.\n");
			exit(0);
		}
	}

	if (pcr) avg=1;
	if (i_average) avg=1;
	if (j_average) avg=1;
	if (k_average) avg=1;
	if (ik_average) avg=1;
	if (ikc_average) avg=1;


	if (i_average + j_average + k_average >1) PetscPrintf(PETSC_COMM_WORLD, "Iavg and Javg cannot be set together !! !\n"), exit(0);

	PetscInt rank, bi;

	PetscMalloc(sizeof(IBMNodes), &ibm0);
	PetscMalloc(sizeof(IBMNodes), &ibm1);

	MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

	PetscPrintf(PETSC_COMM_WORLD, "Before allocation \n");
	// Debug from here
	// Definition of block_number
	if (xyz_input) block_number=1;
	else {
		FILE *fd;
		fd = fopen("grid.dat", "r");
		if (binary_input) fread(&block_number, sizeof(int), 1, fd);
		else fscanf(fd, "%i\n", &block_number);
		MPI_Bcast(&block_number, 1, MPI_INT, 0, PETSC_COMM_WORLD);
		fclose(fd);
	}
	// Debug to here
 
	// Allocate memory to 'user' with type 'UserCtx'
	PetscMalloc(block_number*sizeof(UserCtx), &user);

	PetscOptionsGetInt(PETSC_NULL, "-num_of_particle", &user->num_of_particle, PETSC_NULL);
	PetscOptionsGetInt(PETSC_NULL, "-speedscale", &user->speedscale, PETSC_NULL);
	PetscOptionsGetInt(PETSC_NULL, "-starting_k", &user->starting_k, PETSC_NULL);

	PetscOptionsGetReal(PETSC_NULL, "-ren", &user->ren, PETSC_NULL);
	PetscOptionsGetReal(PETSC_NULL, "-dt", &user->dt, PETSC_NULL);

	//PetscMalloc(block_number*sizeof(PT), &pt);
	// Read grid geometry in 'grid.dat' file
	ReadCoordinates(user);

	PetscPrintf(PETSC_COMM_WORLD, "read coord!\n");

	PetscPrintf(PETSC_COMM_WORLD, "avg = %i \n", avg);     // 0
	PetscPrintf(PETSC_COMM_WORLD, "vc = %i \n", vc);       // 1
	PetscPrintf(PETSC_COMM_WORLD, "QCR = %i \n", QCR);     // 0
	PetscPrintf(PETSC_COMM_WORLD, "pcr = %i \n", pcr);     // 0
	PetscPrintf(PETSC_COMM_WORLD, "shear = %i \n", shear); // 0

	for (bi=0; bi<block_number; bi++) {
		DACreateGlobalVector(user[bi].da, &user[bi].Nvert);
		if (shear) {
			DACreateGlobalVector(user[bi].fda, &user[bi].Csi);
			DACreateGlobalVector(user[bi].fda, &user[bi].Eta);
			DACreateGlobalVector(user[bi].fda, &user[bi].Zet);
			DACreateGlobalVector(user[bi].da, &user[bi].Aj);
			FormMetrics(&(user[bi]));

			Calc_avg_shear_stress(&(user[bi]));

			VecDestroy(user[bi].Csi);
			VecDestroy(user[bi].Eta);
			VecDestroy(user[bi].Zet);
			VecDestroy(user[bi].Aj);
			exit(0);
		}
		else if (!avg) {
			DACreateGlobalVector(user[bi].da, &user[bi].P);
			DACreateGlobalVector(user[bi].fda, &user[bi].Ucat);
			DACreateGlobalVector(user[bi].fda, &user[bi].Pressure_Gradient);
			/*
			DACreateGlobalVector(user[bi].da, &user[bi].xpini);
			DACreateGlobalVector(user[bi].da, &user[bi].ypini);
			DACreateGlobalVector(user[bi].da, &user[bi].zpini);

			DACreateGlobalVector(user[bi].da, &user[bi].xpite);
			DACreateGlobalVector(user[bi].da, &user[bi].ypite);
			DACreateGlobalVector(user[bi].da, &user[bi].zpite);

			DACreateGlobalVector(user[bi].da, &user[bi].uploc);
			DACreateGlobalVector(user[bi].da, &user[bi].vploc);
			DACreateGlobalVector(user[bi].da, &user[bi].wploc);

			DACreateGlobalVector(user[bi].fda, &user[bi].ParLoc);
			DACreateGlobalVector(user[bi].fda, &user[bi].ParVel);

			*/
			DACreateGlobalVector(user[bi].da, &user[bi].Dist);
			if (!vc) DACreateGlobalVector(user[bi].fda, &user[bi].Ucat_o);

			if (QCR)	{
				if (QCR==1 || QCR==2) {
					DACreateGlobalVector(user[bi].fda, &user[bi].Csi);
					DACreateGlobalVector(user[bi].fda, &user[bi].Eta);
					DACreateGlobalVector(user[bi].fda, &user[bi].Zet);
					DACreateGlobalVector(user[bi].da, &user[bi].Aj);
					FormMetrics(&(user[bi]));
				}
			}
		}
		else {
			 if (pcr) {
				DACreateGlobalVector(user[bi].da, &user[bi].P);
				DACreateGlobalVector(user[bi].fda, &user[bi].Ucat);
			}
			else if (avg==1) {
				/*
				DACreateGlobalVector(user[bi].fda, &user[bi].Ucat_sum);
				DACreateGlobalVector(user[bi].fda, &user[bi].Ucat_cross_sum);
				DACreateGlobalVector(user[bi].fda, &user[bi].Ucat_square_sum);
				*/
			}
			else if (avg==2) {	// just compute k
				/*
				DACreateGlobalVector(user[bi].fda, &user[bi].Ucat_sum);
				DACreateGlobalVector(user[bi].fda, &user[bi].Ucat_square_sum);
				*/
			}
		}

	}

	if (avg) {
		if (i_average) PetscPrintf(PETSC_COMM_WORLD, "Averaging in I direction!\n");
		else if (j_average) PetscPrintf(PETSC_COMM_WORLD, "Averaging in J direction!\n");
		else if (k_average) PetscPrintf(PETSC_COMM_WORLD, "Averaging in K direction!\n");
		else if (ik_average) PetscPrintf(PETSC_COMM_WORLD, "Averaging in IK direction!\n");
		else PetscPrintf(PETSC_COMM_WORLD, "Averaging !\n");
		/*
		DACreateGlobalVector(user[bi].fda, &user->Ucat_sum);
		DACreateGlobalVector(user[bi].fda, &user->Ucat_cross_sum);
		DACreateGlobalVector(user[bi].fda, &user->Ucat_square_sum);
		*/

	}

	// Forming the metrics should be done only once
	//
	//
	//


	// Writting data of each block
	for (ti=tis; ti<=tie; ti+=tsteps) {
		for (bi=0; bi<block_number; bi++) {
			if (avg) Ucont_P_Binary_Input_Averaging(&user[bi]); else {
				Ucont_P_Binary_Input(&user[bi]);
				Ucont_P_Binary_Input1(&user[bi]);
			}
		}

		if (!QCR) {
			if (avg) TECIOOut_Averaging(user);
			else if (vtkOutput) {
				VtkOutput(user, onlyV);

/// -------------------------------------------------------------------------------------------------------///


		bi = 0;
		DACreateGlobalVector(user[bi].fda, &user[bi].Csi);
		DACreateGlobalVector(user[bi].fda, &user[bi].Eta);
		DACreateGlobalVector(user[bi].fda, &user[bi].Zet);
		DACreateGlobalVector(user[bi].da, &user[bi].Aj);	
		FormMetrics(&(user[bi]));
	

	///----------------------------------------------------------------//


		      	         if (ti == tis) Step_Injection(user, ti, tis, tsteps); // First step: Injection
		       		// From second step: Iterate equation of motion
		       		 else Step_TimeIteration(user, ti, tis, tsteps);


		VecDestroy(user[bi].Csi);
		VecDestroy(user[bi].Eta);
		VecDestroy(user[bi].Zet);
		VecDestroy(user[bi].Aj);

			
			}
			else TECIOOut_V(user, onlyV);
		}
		else {
			TECIOOutQ(user, QCR);
		}
	}
	PetscFinalize();
}

PetscErrorCode ReadCoordinates(UserCtx *user) {
	Cmpnts ***coor;

	Vec Coor;
	PetscInt bi, i, j, k, rank, IM, JM, KM;
	PetscReal *gc;
	FILE *fd;
	PetscReal	d0 = 1.;

	MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

	PetscReal	cl = 1.;
	PetscOptionsGetReal(PETSC_NULL, "-cl", &cl, PETSC_NULL);

	char str[256];

	if (xyz_input) sprintf(str, "xyz.dat");
	else sprintf(str, "grid.dat");

	fd = fopen(str, "r");

	if (fd==NULL) printf("Cannot open %s !\n", str),exit(0);

	printf("Begin Reading %s !\n", str);

	if (xyz_input) i=1;
	else if (binary_input) {
		fread(&i, sizeof(int), 1, fd);
		if (i!=1) PetscPrintf(PETSC_COMM_WORLD, "This seems to be a text file !\n"),exit(0);
	}
	else {
		fscanf(fd, "%i\n", &i);
		if (i!=1) PetscPrintf(PETSC_COMM_WORLD, "This seems to be a binary file !\n"),exit(0);
	}


	for (bi=block_number-1; bi>=0; bi--) {

		std::vector<double> X, Y,Z;
		double tmp;

		if (xyz_input) {
			fscanf(fd, "%i %i %i\n", &(user[bi].IM), &(user[bi].JM), &(user[bi].KM));
			X.resize(user[bi].IM);
			Y.resize(user[bi].JM);
			Z.resize(user[bi].KM);

			for (i=0; i<user[bi].IM; i++) fscanf(fd, "%le %le %le\n", &X[i], &tmp, &tmp);
			for (j=0; j<user[bi].JM; j++) fscanf(fd, "%le %le %le\n", &tmp, &Y[j], &tmp);
			for (k=0; k<user[bi].KM; k++) fscanf(fd, "%le %le %le\n", &tmp, &tmp, &Z[k]);
		}
		else if (binary_input) {
			fread(&(user[bi].IM), sizeof(int), 1, fd);
			fread(&(user[bi].JM), sizeof(int), 1, fd);
			fread(&(user[bi].KM), sizeof(int), 1, fd);
		}
		else fscanf(fd, "%i %i %i\n", &(user[bi].IM), &(user[bi].JM), &(user[bi].KM));

		IM = user[bi].IM; JM = user[bi].JM; KM = user[bi].KM;


		DACreate3d(PETSC_COMM_WORLD, DA_NONPERIODIC, DA_STENCIL_BOX,
			user[bi].IM+1, user[bi].JM+1, user[bi].KM+1, 1,1,
			PETSC_DECIDE, 1, 2, PETSC_NULL, PETSC_NULL, PETSC_NULL,
			&(user[bi].da));
		if (rans) {
			DACreate3d(PETSC_COMM_WORLD, DA_NONPERIODIC, DA_STENCIL_BOX,
				user[bi].IM+1, user[bi].JM+1, user[bi].KM+1, 1,1,
				PETSC_DECIDE, 2, 2, PETSC_NULL, PETSC_NULL, PETSC_NULL,
				&(user[bi].fda2));
		}
		DASetUniformCoordinates(user[bi].da, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0);
		DAGetCoordinateDA(user[bi].da, &(user[bi].fda));

		DAGetLocalInfo(user[bi].da, &(user[bi].info));

		DALocalInfo	info = user[bi].info;
		PetscInt	xs = info.xs, xe = info.xs + info.xm;
		PetscInt  	ys = info.ys, ye = info.ys + info.ym;
		PetscInt	zs = info.zs, ze = info.zs + info.zm;
		PetscInt	mx = info.mx, my = info.my, mz = info.mz;

		DAGetGhostedCoordinates(user[bi].da, &Coor);
		DAVecGetArray(user[bi].fda, Coor, &coor);

		double buffer;

		for (k=0; k<KM; k++)
		for (j=0; j<JM; j++)
		for (i=0; i<IM; i++) {
			if (xyz_input) ;
			else if (binary_input) fread(&buffer, sizeof(double), 1, fd);
			else fscanf(fd, "%le", &buffer);

			if ( k>=zs && k<=ze && j>=ys && j<ye && i>=xs && i<xe ) {
				if (xyz_input) coor[k][j][i].x = X[i]/cl;
				else coor[k][j][i].x = buffer/cl;
			}
		}

		for (k=0; k<KM; k++)
		for (j=0; j<JM; j++)
		for (i=0; i<IM; i++) {
			if (xyz_input) ;
			else if (binary_input) fread(&buffer, sizeof(double), 1, fd);
			else fscanf(fd, "%le", &buffer);

			if ( k>=zs && k<=ze && j>=ys && j<ye && i>=xs && i<xe ) {
				if (xyz_input) coor[k][j][i].y = Y[j]/cl;
				else coor[k][j][i].y = buffer/cl;
			}
		}

		for (k=0; k<KM; k++)
		for (j=0; j<JM; j++)
		for (i=0; i<IM; i++) {
			if (xyz_input) ;
			else if (binary_input) fread(&buffer, sizeof(double), 1, fd);
			else fscanf(fd, "%le", &buffer);

			if ( k>=zs && k<=ze && j>=ys && j<ye && i>=xs && i<xe ) {
				if (xyz_input) coor[k][j][i].z = Z[k]/cl;
				else coor[k][j][i].z = buffer/cl;
			}
		}

		DAVecRestoreArray(user[bi].fda, Coor, &coor);

		Vec	gCoor;
		DAGetCoordinates(user[bi].da, &gCoor);
		DALocalToGlobal(user[bi].fda, Coor, INSERT_VALUES, gCoor);
		DAGlobalToLocalBegin(user[bi].fda, gCoor, INSERT_VALUES, Coor);
		DAGlobalToLocalEnd(user[bi].fda, gCoor, INSERT_VALUES, Coor);

	}

	fclose(fd);

	printf("Finish Reading %s !\n", str);

	for (bi=0; bi<block_number; bi++) {
		user[bi]._this = bi;
	}
	return(0);
}

void Calc_avg_shear_stress(UserCtx *user)
{
	double N=(double)tis+1.0;
	DALocalInfo	info = user->info;
	PetscInt	xs = info.xs, xe = info.xs + info.xm;
	PetscInt  	ys = info.ys, ye = info.ys + info.ym;
	PetscInt	zs = info.zs, ze = info.zs + info.zm;
	PetscInt	mx = info.mx, my = info.my, mz = info.mz;

	PetscInt	lxs, lxe, lys, lye, lzs, lze;

	PetscInt i, j, k;
	lxs = xs; lxe = xe; lys = ys; lye = ye; lzs = zs; lze = ze;

	Cmpnts ***usum, ***csi, ***eta, ***zet;
	PetscReal ***aj, ***psum, ***nvert;
	if (lxs==0) lxs++;
	if (lxe==mx) lxe--;
	if (lys==0) lys++;
	if (lye==my) lye--;
	if (lzs==0) lzs++;
	if (lze==mz) lze--;

	char filen[256];
	PetscViewer	viewer;

	Vec P_sum;
	DACreateGlobalVector(user->da, &P_sum);
	DACreateGlobalVector(user->fda, &user->Ucat_sum);

	ti=tis;
	sprintf(filen, "su0_%05d_%1d.dat", ti, user->_this);
	PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
	VecLoadIntoVector(viewer, (user->Ucat_sum));
	PetscViewerDestroy(viewer);

	ti=tis;
	sprintf(filen, "sp_%05d_%1d.dat", ti, user->_this);
	PetscViewerBinaryOpen(PETSC_COMM_WORLD, filen, FILE_MODE_READ, &viewer);
	VecLoadIntoVector(viewer, P_sum);
	PetscViewerDestroy(viewer);

	DAVecGetArray(user->fda, user->Csi, &csi);
	DAVecGetArray(user->fda, user->Eta, &eta);
	DAVecGetArray(user->fda, user->Zet, &zet);
	DAVecGetArray(user->da, user->Aj, &aj);
	DAVecGetArray(user->da, user->Nvert, &nvert);
	DAVecGetArray(user->fda, user->Ucat_sum, &usum);
	DAVecGetArray(user->da, P_sum, &psum);


	double force_skin_bottom = 0;
	double force_pressure_bottom = 0;
	double force_bottom = 0;
	double area_bottom = 0;

	double force_skin_top = 0;
	double force_pressure_top = 0;
	double force_top = 0;
	double area_top = 0;

	j=0;
	for (k=lzs; k<lze; k++)
	for (i=lxs; i<lxe; i++) {
		if (nvert[k][j+1][i] < 0.1) {
			double du_dx, du_dy, du_dz, dv_dx, dv_dy, dv_dz, dw_dx, dw_dy, dw_dz;
			double dudc, dvdc, dwdc, dude, dvde, dwde, dudz, dvdz, dwdz;

			dudc=0, dvdc=0, dwdc=0;

			dude=usum[k][j+1][i].x * 2.0 / N;
			dvde=usum[k][j+1][i].y * 2.0 / N;
			dwde=usum[k][j+1][i].z * 2.0 / N;

			dudz=0, dvdz=0, dwdz=0;

			double ajc = aj[k][j+1][i];
			double csi0 = csi[k][j+1][i].x, csi1 = csi[k][j+1][i].y, csi2 = csi[k][j+1][i].z;
			double eta0 = eta[k][j+1][i].x, eta1 = eta[k][j+1][i].y, eta2 = eta[k][j+1][i].z;
			double zet0 = zet[k][j+1][i].x, zet1 = zet[k][j+1][i].y, zet2 = zet[k][j+1][i].z;

			Compute_du_dxyz (csi0, csi1, csi2, eta0, eta1, eta2, zet0, zet1, zet2, ajc,
					dudc, dvdc, dwdc, dude, dvde, dwde, dudz, dvdz, dwdz,
					&du_dx, &dv_dx, &dw_dx, &du_dy, &dv_dy, &dw_dy, &du_dz, &dv_dz, &dw_dz );

			double j_area = sqrt( eta[k][j+1][i].x*eta[k][j+1][i].x + eta[k][j+1][i].y*eta[k][j+1][i].y + eta[k][j+1][i].z*eta[k][j+1][i].z );
			double ni[3], nj[3], nk[3];
			double nx, ny, nz;
			Calculate_normal(csi[k][j+1][i], eta[k][j+1][i], zet[k][j+1][i], ni, nj, nk);
			nx = nj[0]; //inward normal
			ny = nj[1]; //inward normal
			nz = nj[2]; //inward normal


			double Fp = - psum[k][j+1][i] * eta2 / N;
			double Fs = (dw_dx * nx + dw_dy * ny + dw_dz * nz) / user->ren * j_area;
			//double Fs = (du_dx * nx + du_dy * ny + du_dz * nz) / user->ren * j_area;

			force_skin_bottom += Fs;
			force_pressure_bottom += Fp;
			force_bottom += Fs + Fp;
			area_bottom += fabs(eta1);	// projected area
		}
	}

	j=my-2;
	for (k=lzs; k<lze; k++)
	for (i=lxs; i<lxe; i++) {
		if (nvert[k][j][i] < 0.1) {
			double du_dx, du_dy, du_dz, dv_dx, dv_dy, dv_dz, dw_dx, dw_dy, dw_dz;
			double dudc, dvdc, dwdc, dude, dvde, dwde, dudz, dvdz, dwdz;

			dudc=0, dvdc=0, dwdc=0;

			dude = -usum[k][j][i].x * 2.0 / N;
			dvde = -usum[k][j][i].y * 2.0 / N;
			dwde = -usum[k][j][i].z * 2.0 / N;

			dudz=0, dvdz=0, dwdz=0;

			double ajc = aj[k][j][i];
			double csi0 = csi[k][j][i].x, csi1 = csi[k][j][i].y, csi2 = csi[k][j][i].z;
			double eta0 = eta[k][j][i].x, eta1 = eta[k][j][i].y, eta2 = eta[k][j][i].z;
			double zet0 = zet[k][j][i].x, zet1 = zet[k][j][i].y, zet2 = zet[k][j][i].z;

			Compute_du_dxyz (csi0, csi1, csi2, eta0, eta1, eta2, zet0, zet1, zet2, ajc,
					dudc, dvdc, dwdc, dude, dvde, dwde, dudz, dvdz, dwdz,
					&du_dx, &dv_dx, &dw_dx, &du_dy, &dv_dy, &dw_dy, &du_dz, &dv_dz, &dw_dz );

			double j_area = sqrt( eta[k][j][i].x*eta[k][j][i].x + eta[k][j][i].y*eta[k][j][i].y + eta[k][j][i].z*eta[k][j][i].z );
			double ni[3], nj[3], nk[3];
			double nx, ny, nz;
			Calculate_normal(csi[k][j][i], eta[k][j][i], zet[k][j][i], ni, nj, nk);
			nx = -nj[0]; //inward normal
			ny = -nj[1]; //inward normal
			nz = -nj[2]; //inward normal


			double Fp = - psum[k][j][i] * eta2 / N;
			double Fs = (dw_dx * nx + dw_dy * ny + dw_dz * nz) / user->ren * j_area;
			//double Fs = (du_dx * nx + du_dy * ny + du_dz * nz) / user->ren * j_area;

			force_skin_top += Fs;
			force_pressure_top += Fp;
			force_top += Fs + Fp;
			area_top += fabs(eta1);	// projected area
		}
	}

	DAVecRestoreArray(user->fda, user->Csi, &csi);
	DAVecRestoreArray(user->fda, user->Eta, &eta);
	DAVecRestoreArray(user->fda, user->Zet, &zet);
	DAVecRestoreArray(user->da, user->Aj, &aj);
	DAVecRestoreArray(user->da, user->Nvert, &nvert);
	DAVecRestoreArray(user->fda, user->Ucat_sum, &usum);
	DAVecRestoreArray(user->da, P_sum, &psum);

	VecDestroy(P_sum);
	VecDestroy(user->Ucat_sum);

	printf("Top:\tarea=%f, force=%f, skin force=%f, pressure force=%f\n",
				area_top, force_top, force_skin_top, force_pressure_top);

	printf("\tstress=%f, skin stress=%f, pressure stress=%f, u*=%f, Re*=%f\n",
				force_top/area_top, force_skin_top/area_top, force_pressure_top/area_top,
				sqrt(fabs(force_top/area_top)), sqrt(fabs(force_top/area_top))*user->ren);

	printf("\n");

	printf("Bottom:\tarea=%f, force=%f, skin force=%f, pressure force=%f\n",
				area_bottom, force_bottom, force_skin_bottom, force_pressure_bottom);

	printf("\tstress=%f, skin stress=%f, pressure stress=%f, u*=%f, Re*=%f\n",
				force_bottom/area_bottom, force_skin_bottom/area_bottom, force_pressure_bottom/area_bottom,
				sqrt(fabs(force_bottom/area_bottom)), sqrt(fabs(force_bottom/area_bottom))*user->ren);
}

PetscErrorCode OneParticle(UserCtx *user)
{
	DALocalInfo	info = user->info;
	PetscInt	xs = info.xs, xe = info.xs + info.xm;
	PetscInt  	ys = info.ys, ye = info.ys + info.ym;
	PetscInt	zs = info.zs, ze = info.zs + info.zm;
	PetscInt	mx = info.mx, my = info.my, mz = info.mz;

	PetscInt	lxs, lxe, lys, lye, lzs, lze;

	PetscInt i, j, k;
	lxs = xs; lxe = xe; lys = ys; lye = ye; lzs = zs; lze = ze;

	Cmpnts ***ucat, ***csi, ***eta, ***zet;
	PetscReal ***aj, ***q, ***nvert;
	if (lxs==0) lxs++;
	if (lxe==mx) lxe--;
	if (lys==0) lys++;
	if (lye==my) lye--;
	if (lzs==0) lzs++;
	if (lze==mz) lze--;

	DAVecGetArray(user->fda, user->Ucat, &ucat);
	DAVecGetArray(user->fda, user->Csi, &csi);
	DAVecGetArray(user->fda, user->Eta, &eta);
	DAVecGetArray(user->fda, user->Zet, &zet);

	DAVecGetArray(user->da, user->Aj, &aj);
	DAVecGetArray(user->da, user->Nvert, &nvert);
	DAVecGetArray(user->da, user->P, &q);

	PetscReal uc, vc, wc, ue, ve, we, uz, vz, wz;

	PetscReal s11, s12, s13, s21, s22, s23, s31, s32, s33;
	PetscReal d11, d12, d13, d21, d22, d23, d31, d32, d33;

	PetscReal w11, w12, w13, w21, w22, w23, w31, w32, w33;
	//PetscReal so, wo;
	PetscReal csi1, csi2, csi3, eta1, eta2, eta3, zet1, zet2, zet3;
	for (k=lzs; k<lze; k++) {
		for (j=lys; j<lye; j++) {
			for (i=lxs; i<lxe; i++) {

				double dudc, dvdc, dwdc, dude, dvde, dwde, dudz, dvdz, dwdz;
				double du_dx, du_dy, du_dz, dv_dx, dv_dy, dv_dz, dw_dx, dw_dy, dw_dz;
				double csi0 = csi[k][j][i].x, csi1 = csi[k][j][i].y, csi2 = csi[k][j][i].z;
				double eta0 = eta[k][j][i].x, eta1 = eta[k][j][i].y, eta2 = eta[k][j][i].z;
				double zet0 = zet[k][j][i].x, zet1 = zet[k][j][i].y, zet2 = zet[k][j][i].z;
				double ajc  = aj[k][j][i];

				double upar_mag, upar_x, upar_y, upar_z;
				double xp, yp, zp;

				// Average Function

				//upar_x = (ucat[k][j][i].x + ucat[k][j][i+1].x + ucat[k][j+1][i].x + ucat[k][j+1][i+1].x + ucat[k+1][j][i].x + ucat[k+1][j][i+1].x ucat[k+1][j+1][i].x + ucat[k+1][j+1][i+1].x) * 0.125;

				//upar_y = (ucat[k][j][i].y + ucat[k][j][i+1].y + ucat[k][j+1][i].y + ucat[k][j+1][i+1].y + ucat[k+1][j][i].y + ucat[k+1][j][i+1].y ucat[k+1][j+1][i].y + ucat[k+1][j+1][i+1].y) * 0.125;

				//upar_z = (ucat[k][j][i].z + ucat[k][j][i+1].z + ucat[k][j+1][i].z + ucat[k][j+1][i+1].z + ucat[k+1][j][i].z + ucat[k+1][j][i+1].z ucat[k+1][j+1][i].z + ucat[k+1][j+1][i+1].z) * 0.125;

				//upar_mag = sqrt( upar_x*upar_x + upar_y*upar_y + upar_z*upar_z );

			}
		}
	}

	DAVecRestoreArray(user->fda, user->Ucat, &ucat);
	DAVecRestoreArray(user->fda, user->Csi, &csi);
	DAVecRestoreArray(user->fda, user->Eta, &eta);
	DAVecRestoreArray(user->fda, user->Zet, &zet);

	DAVecRestoreArray(user->da, user->Aj, &aj);
	DAVecRestoreArray(user->da, user->Nvert, &nvert);
	DAVecRestoreArray(user->da, user->P, &q);

	return 0;

}

PetscErrorCode Lambda2(UserCtx *user)
{
	DALocalInfo	info = user->info;
	PetscInt	xs = info.xs, xe = info.xs + info.xm;
	PetscInt  	ys = info.ys, ye = info.ys + info.ym;
	PetscInt	zs = info.zs, ze = info.zs + info.zm;
	PetscInt	mx = info.mx, my = info.my, mz = info.mz;

	PetscInt	lxs, lxe, lys, lye, lzs, lze;

	PetscInt i, j, k;
	lxs = xs; lxe = xe; lys = ys; lye = ye; lzs = zs; lze = ze;

	Cmpnts ***ucat, ***csi, ***eta, ***zet;
	PetscReal ***aj, ***q, ***nvert;
	if (lxs==0) lxs++;
	if (lxe==mx) lxe--;
	if (lys==0) lys++;
	if (lye==my) lye--;
	if (lzs==0) lzs++;
	if (lze==mz) lze--;

	DAVecGetArray(user->fda, user->Ucat, &ucat);
	DAVecGetArray(user->fda, user->Csi, &csi);
	DAVecGetArray(user->fda, user->Eta, &eta);
	DAVecGetArray(user->fda, user->Zet, &zet);

	DAVecGetArray(user->da, user->Aj, &aj);
	DAVecGetArray(user->da, user->Nvert, &nvert);
	DAVecGetArray(user->da, user->P, &q);

	PetscReal uc, vc, wc, ue, ve, we, uz, vz, wz;

	PetscReal s11, s12, s13, s21, s22, s23, s31, s32, s33;
	PetscReal d11, d12, d13, d21, d22, d23, d31, d32, d33;

	PetscReal w11, w12, w13, w21, w22, w23, w31, w32, w33;
	//PetscReal so, wo;
	PetscReal csi1, csi2, csi3, eta1, eta2, eta3, zet1, zet2, zet3;
	for (k=lzs; k<lze; k++) {
		for (j=lys; j<lye; j++) {
			for (i=lxs; i<lxe; i++) {

				double dudc, dvdc, dwdc, dude, dvde, dwde, dudz, dvdz, dwdz;
				double du_dx, du_dy, du_dz, dv_dx, dv_dy, dv_dz, dw_dx, dw_dy, dw_dz;
				double csi0 = csi[k][j][i].x, csi1 = csi[k][j][i].y, csi2 = csi[k][j][i].z;
				double eta0= eta[k][j][i].x, eta1 = eta[k][j][i].y, eta2 = eta[k][j][i].z;
				double zet0 = zet[k][j][i].x, zet1 = zet[k][j][i].y, zet2 = zet[k][j][i].z;
				double ajc = aj[k][j][i];

				Compute_du_center (i, j, k, mx, my, mz, ucat, nvert, &dudc, &dvdc, &dwdc, &dude, &dvde, &dwde, &dudz, &dvdz, &dwdz);
				Compute_du_dxyz (	csi0, csi1, csi2, eta0, eta1, eta2, zet0, zet1, zet2, ajc, dudc, dvdc, dwdc, dude, dvde, dwde, dudz, dvdz, dwdz,
										&du_dx, &dv_dx, &dw_dx, &du_dy, &dv_dy, &dw_dy, &du_dz, &dv_dz, &dw_dz );

				double Sxx = 0.5*( du_dx + du_dx ), Sxy = 0.5*(du_dy + dv_dx), Sxz = 0.5*(du_dz + dw_dx);
				double Syx = Sxy, Syy = 0.5*(dv_dy + dv_dy),	Syz = 0.5*(dv_dz + dw_dy);
				double Szx = Sxz, Szy=Syz, Szz = 0.5*(dw_dz + dw_dz);


				w11 = 0;
				w12 = 0.5*(du_dy - dv_dx);
				w13 = 0.5*(du_dz - dw_dx);
				w21 = -w12;
				w22 = 0.;
				w23 = 0.5*(dv_dz - dw_dy);
				w31 = -w13;
				w32 = -w23;
				w33 = 0.;


				double S[3][3], W[3][3], D[3][3];

				D[0][0] = du_dx, D[0][1] = du_dy, D[0][2] = du_dz;
				D[1][0] = dv_dx, D[1][1] = dv_dy, D[1][2] = dv_dz;
				D[2][0] = dw_dx, D[2][1] = dw_dy, D[2][2] = dw_dz;

				S[0][0] = Sxx;
				S[0][1] = Sxy;
				S[0][2] = Sxz;

				S[1][0] = Syx;
				S[1][1] = Syy;
				S[1][2] = Syz;

				S[2][0] = Szx;
				S[2][1] = Szy;
				S[2][2] = Szz;

				W[0][0] = w11;
				W[0][1] = w12;
				W[0][2] = w13;
				W[1][0] = w21;
				W[1][1] = w22;
				W[1][2] = w23;
				W[2][0] = w31;
				W[2][1] = w32;
				W[2][2] = w33;

				// lambda-2
				double A[3][3], V[3][3], d[3];

				for (int row=0; row<3; row++)
				for (int col=0; col<3; col++) A[row][col]=0;

				for (int row=0; row<3; row++)
				for (int col=0; col<3; col++) {
					A[row][col] += S[row][0] * S[0][col];
					A[row][col] += S[row][1] * S[1][col];
					A[row][col] += S[row][2] * S[2][col];
				}

				for (int row=0; row<3; row++)
				for (int col=0; col<3; col++) {
					A[row][col] += W[row][0] * W[0][col];
					A[row][col] += W[row][1] * W[1][col];
					A[row][col] += W[row][2] * W[2][col];
				}

				if (nvert[k][j][i]<0.1) {
					eigen_decomposition(A, V, d);
					q[k][j][i] = d[1];
				}
				else q[k][j][i] = 1000.0;
/*
				// delta criterion
				double DD[3][3];
				for (int row=0; row<3; row++)
				for (int col=0; col<3; col++) DD[row][col]=0;

				for (int row=0; row<3; row++)
				for (int col=0; col<3; col++) {
					DD[row][col] += D[row][0] * D[0][col];
					DD[row][col] += D[row][1] * D[1][col];
					DD[row][col] += D[row][2] * D[2][col];
				}
				double tr_DD = DD[0][0] + DD[1][1] + DD[2][2];
				double det_D = D[0][0]*(D[2][2]*D[1][1]-D[2][1]*D[1][2])-D[1][0]*(D[2][2]*D[0][1]-D[2][1]*D[0][2])+D[2][0]*(D[1][2]*D[0][1]-D[1][1]*D[0][2]);

				//double Q = -0.5*tr_DD;

				double SS=0, WW=0;
				for (int row=0; row<3; row++)
				for (int col=0; col<3; col++) {
					SS+=S[row][col]*S[row][col];
					WW+=W[row][col]*W[row][col];
				}
				double Q = 0.5*(WW - SS);

				double R = - det_D;
				if (nvert[k][j][i]<0.1) {
					q[k][j][i] = pow( 0.5*R, 2. )  + pow( Q/3., 3.);
				}
				else q[k][j][i] = -10;
				if (q[k][j][i]<0) q[k][j][i]=-10;
*/
			}
		}
	}

	DAVecRestoreArray(user->fda, user->Ucat, &ucat);
	DAVecRestoreArray(user->fda, user->Csi, &csi);
	DAVecRestoreArray(user->fda, user->Eta, &eta);
	DAVecRestoreArray(user->fda, user->Zet, &zet);

	DAVecRestoreArray(user->da, user->Aj, &aj);
	DAVecRestoreArray(user->da, user->Nvert, &nvert);
	DAVecRestoreArray(user->da, user->P, &q);

	return 0;
}

PetscErrorCode QCriteria(UserCtx *user)
{

	DALocalInfo	info = user->info;
	PetscInt	xs = info.xs, xe = info.xs + info.xm;
	PetscInt  	ys = info.ys, ye = info.ys + info.ym;
	PetscInt	zs = info.zs, ze = info.zs + info.zm;
	PetscInt	mx = info.mx, my = info.my, mz = info.mz;

	PetscInt	lxs, lxe, lys, lye, lzs, lze;

	PetscInt i, j, k;
	lxs = xs; lxe = xe; lys = ys; lye = ye; lzs = zs; lze = ze;

	Cmpnts ***ucat, ***csi, ***eta, ***zet;
	PetscReal ***aj, ***q, ***nvert;
	if (lxs==0) lxs++;
	if (lxe==mx) lxe--;
	if (lys==0) lys++;
	if (lye==my) lye--;
	if (lzs==0) lzs++;
	if (lze==mz) lze--;

	DAVecGetArray(user->fda, user->Ucat, &ucat);
	DAVecGetArray(user->fda, user->Csi, &csi);
	DAVecGetArray(user->fda, user->Eta, &eta);
	DAVecGetArray(user->fda, user->Zet, &zet);

	DAVecGetArray(user->da, user->Aj, &aj);
	DAVecGetArray(user->da, user->Nvert, &nvert);
	DAVecGetArray(user->da, user->P, &q);

	PetscReal uc, vc, wc, ue, ve, we, uz, vz, wz;

	PetscReal s11, s12, s13, s21, s22, s23, s31, s32, s33;
	PetscReal d11, d12, d13, d21, d22, d23, d31, d32, d33;

	PetscReal w11, w12, w13, w21, w22, w23, w31, w32, w33;
	PetscReal so, wo;
	PetscReal csi1, csi2, csi3, eta1, eta2, eta3, zet1, zet2, zet3;


	for (k=lzs; k<lze; k++) {
		for (j=lys; j<lye; j++) {
			for (i=lxs; i<lxe; i++) {
				double dudc, dvdc, dwdc, dude, dvde, dwde, dudz, dvdz, dwdz;
				double du_dx, du_dy, du_dz, dv_dx, dv_dy, dv_dz, dw_dx, dw_dy, dw_dz;
				double csi0 = csi[k][j][i].x, csi1 = csi[k][j][i].y, csi2 = csi[k][j][i].z;
				double eta0= eta[k][j][i].x, eta1 = eta[k][j][i].y, eta2 = eta[k][j][i].z;
				double zet0 = zet[k][j][i].x, zet1 = zet[k][j][i].y, zet2 = zet[k][j][i].z;
				double ajc = aj[k][j][i];

				Compute_du_center (i, j, k, mx, my, mz, ucat, nvert, &dudc, &dvdc, &dwdc, &dude, &dvde, &dwde, &dudz, &dvdz, &dwdz);
				Compute_du_dxyz (	csi0, csi1, csi2, eta0, eta1, eta2, zet0, zet1, zet2, ajc, dudc, dvdc, dwdc, dude, dvde, dwde, dudz, dvdz, dwdz,
										&du_dx, &dv_dx, &dw_dx, &du_dy, &dv_dy, &dw_dy, &du_dz, &dv_dz, &dw_dz );

				double Sxx = 0.5*( du_dx + du_dx ), Sxy = 0.5*(du_dy + dv_dx), Sxz = 0.5*(du_dz + dw_dx);
				double Syx = Sxy, Syy = 0.5*(dv_dy + dv_dy),	Syz = 0.5*(dv_dz + dw_dy);
				double Szx = Sxz, Szy=Syz, Szz = 0.5*(dw_dz + dw_dz);
				so = Sxx*Sxx + Sxy*Sxy + Sxz*Sxz + Syx*Syx + Syy*Syy + Syz*Syz + Szx*Szx + Szy*Szy + Szz*Szz;

				w11 = 0;
				w12 = 0.5*(du_dy - dv_dx);
				w13 = 0.5*(du_dz - dw_dx);
				w21 = -w12;
				w22 = 0.;
				w23 = 0.5*(dv_dz - dw_dy);
				w31 = -w13;
				w32 = -w23;
				w33 = 0.;

				wo = w11*w11 + w12*w12 + w13*w13 + w21*w21 + w22*w22 + w23*w23 + w31*w31 + w32*w32 + w33*w33;

/*
				so = ( d11 *  d11 + d22 * d22 + d33 * d33) + 0.5* ( (d12 + d21) * (d12 + d21) + (d13 + d31) * (d13 + d31) + (d23 + d32) * (d23 + d32) );
				wo = 0.5 * ( (d12 - d21)*(d12 - d21) + (d13 - d31) * (d13 - d31) + (d23 - d32) * (d23 - d32) );
				V19=0.5 * ( (V13 - V11)*(V13 - V11) + (V16 - V12) * (V16 - V12) + (V17 - V15) * (V17 - V15) ) - 0.5 * ( V10 *  V10 + V14 * V14 + V18 * V18) - 0.25* ( (V13 + V11) * (V13 + V11) + (V16 + V12) * (V16 + V12) + (V17 + V15) * (V17 + V15) )
*/

				if ( nvert[k][j][i]>0.1 ) q[k][j][i] = -100;
				else q[k][j][i] = (wo - so) / 2.;
			}
		}
	}

	DAVecRestoreArray(user->fda, user->Ucat, &ucat);
	DAVecRestoreArray(user->fda, user->Csi, &csi);
	DAVecRestoreArray(user->fda, user->Eta, &eta);
	DAVecRestoreArray(user->fda, user->Zet, &zet);

	DAVecRestoreArray(user->da, user->Aj, &aj);
	DAVecRestoreArray(user->da, user->Nvert, &nvert);
	DAVecRestoreArray(user->da, user->P, &q);

	return 0;
}

PetscErrorCode Velocity_Magnitude(UserCtx *user)	// store at P
{
	DALocalInfo	info = user->info;
	PetscInt	xs = info.xs, xe = info.xs + info.xm;
	PetscInt  	ys = info.ys, ye = info.ys + info.ym;
	PetscInt	zs = info.zs, ze = info.zs + info.zm;
	PetscInt	mx = info.mx, my = info.my, mz = info.mz;

	PetscInt	lxs, lxe, lys, lye, lzs, lze;

	PetscInt i, j, k;
	lxs = xs; lxe = xe; lys = ys; lye = ye; lzs = zs; lze = ze;

	Cmpnts ***ucat;
	PetscReal ***aj, ***q, ***nvert;
	if (lxs==0) lxs++;
	if (lxe==mx) lxe--;
	if (lys==0) lys++;
	if (lye==my) lye--;
	if (lzs==0) lzs++;
	if (lze==mz) lze--;

	DAVecGetArray(user->fda, user->Ucat, &ucat);
	DAVecGetArray(user->da, user->P, &q);

	PetscReal uc, vc, wc, ue, ve, we, uz, vz, wz;
	PetscReal d11, d12, d13, d21, d22, d23, d31, d32, d33;

	for (k=lzs; k<lze; k++) {
		for (j=lys; j<lye; j++) {
			for (i=lxs; i<lxe; i++) {
				q[k][j][i] = sqrt( ucat[k][j][i].x*ucat[k][j][i].x + ucat[k][j][i].y*ucat[k][j][i].y + ucat[k][j][i].z*ucat[k][j][i].z );
			}
		}
	}

	DAVecRestoreArray(user->fda, user->Ucat, &ucat);
	DAVecRestoreArray(user->da, user->P, &q);

	return 0;
}


PetscErrorCode ibm_read(IBMNodes *ibm)
{
	PetscInt	rank;
	PetscInt	n_v , n_elmt ;
	PetscReal	*x_bp , *y_bp , *z_bp ;
	PetscInt	*nv1 , *nv2 , *nv3 ;
	PetscReal	*nf_x, *nf_y, *nf_z;
	PetscInt	i;
	PetscInt	n1e, n2e, n3e;
	PetscReal	dx12, dy12, dz12, dx13, dy13, dz13;
	PetscReal	t, dr;
	double xt;
	//MPI_Comm_size(PETSC_COMM_WORLD, &size);
	MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

	if (!rank) { // root processor read in the data
		FILE *fd;
		fd = fopen("ibmdata0", "r"); if (!fd) SETERRQ(1, "Cannot open IBM node file")
		n_v =0;
		fscanf(fd, "%i", &n_v);
		fscanf(fd, "%i", &n_v);
		fscanf(fd, "%le", &xt);
		ibm->n_v = n_v;

		MPI_Bcast(&(ibm->n_v), 1, MPI_INT, 0, PETSC_COMM_WORLD);
		//    PetscPrintf(PETSC_COMM_WORLD, "nv, %d %e \n", n_v, xt);
		PetscMalloc(n_v*sizeof(PetscReal), &x_bp);
		PetscMalloc(n_v*sizeof(PetscReal), &y_bp);
		PetscMalloc(n_v*sizeof(PetscReal), &z_bp);

		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->x_bp));
		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->y_bp));
		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->z_bp));

		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->x_bp_o));
		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->y_bp_o));
		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->z_bp_o));

		PetscMalloc(n_v*sizeof(Cmpnts), &(ibm->u));
		PetscMalloc(n_v*sizeof(Cmpnts), &(ibm->uold));

		for (i=0; i<n_v; i++) {
			fscanf(fd, "%le %le %le %le %le %le", &x_bp[i], &y_bp[i], &z_bp[i], &t, &t, &t);
			x_bp[i] = x_bp[i] / 28.;
			y_bp[i] = y_bp[i] / 28.;
			z_bp[i] = z_bp[i] / 28.;
		}
		ibm->x_bp0 = x_bp; ibm->y_bp0 = y_bp; ibm->z_bp0 = z_bp;

		for (i=0; i<n_v; i++) {
			PetscReal temp;
			temp = ibm->y_bp0[i];
			ibm->y_bp0[i] = ibm->z_bp0[i];
			ibm->z_bp0[i] = -temp;
		}


		MPI_Bcast(ibm->x_bp0, n_v, MPIU_REAL, 0, PETSC_COMM_WORLD);
		MPI_Bcast(ibm->y_bp0, n_v, MPIU_REAL, 0, PETSC_COMM_WORLD);
		MPI_Bcast(ibm->z_bp0, n_v, MPIU_REAL, 0, PETSC_COMM_WORLD);

		fscanf(fd, "%i\n", &n_elmt);
		ibm->n_elmt = n_elmt;
		MPI_Bcast(&(ibm->n_elmt), 1, MPI_INT, 0, PETSC_COMM_WORLD);

		PetscMalloc(n_elmt*sizeof(PetscInt), &nv1);
		PetscMalloc(n_elmt*sizeof(PetscInt), &nv2);
		PetscMalloc(n_elmt*sizeof(PetscInt), &nv3);

		PetscMalloc(n_elmt*sizeof(PetscReal), &nf_x);
		PetscMalloc(n_elmt*sizeof(PetscReal), &nf_y);
		PetscMalloc(n_elmt*sizeof(PetscReal), &nf_z);
		for (i=0; i<n_elmt; i++) {

			fscanf(fd, "%i %i %i\n", nv1+i, nv2+i, nv3+i);
			nv1[i] = nv1[i] - 1; nv2[i] = nv2[i]-1; nv3[i] = nv3[i] - 1;
			//      PetscPrintf(PETSC_COMM_WORLD, "I %d %d %d\n", nv1[i], nv2[i], nv3[i]);
		}
		ibm->nv1 = nv1; ibm->nv2 = nv2; ibm->nv3 = nv3;

		fclose(fd);

		for (i=0; i<n_elmt; i++) {
			n1e = nv1[i]; n2e =nv2[i]; n3e = nv3[i];
			dx12 = x_bp[n2e] - x_bp[n1e];
			dy12 = y_bp[n2e] - y_bp[n1e];
			dz12 = z_bp[n2e] - z_bp[n1e];

			dx13 = x_bp[n3e] - x_bp[n1e];
			dy13 = y_bp[n3e] - y_bp[n1e];
			dz13 = z_bp[n3e] - z_bp[n1e];

			nf_x[i] = dy12 * dz13 - dz12 * dy13;
			nf_y[i] = -dx12 * dz13 + dz12 * dx13;
			nf_z[i] = dx12 * dy13 - dy12 * dx13;

			dr = sqrt(nf_x[i]*nf_x[i] + nf_y[i]*nf_y[i] + nf_z[i]*nf_z[i]);

			nf_x[i] /=dr; nf_y[i]/=dr; nf_z[i]/=dr;
		}

		ibm->nf_x = nf_x; ibm->nf_y = nf_y;  ibm->nf_z = nf_z;

/*     for (i=0; i<n_elmt; i++) { */
/*       PetscPrintf(PETSC_COMM_WORLD, "%d %d %d %d\n", i, nv1[i], nv2[i], nv3[i]); */
/*     } */
		MPI_Bcast(ibm->nv1, n_elmt, MPI_INT, 0, PETSC_COMM_WORLD);
		MPI_Bcast(ibm->nv2, n_elmt, MPI_INT, 0, PETSC_COMM_WORLD);
		MPI_Bcast(ibm->nv3, n_elmt, MPI_INT, 0, PETSC_COMM_WORLD);

		MPI_Bcast(ibm->nf_x, n_elmt, MPIU_REAL, 0, PETSC_COMM_WORLD);
		MPI_Bcast(ibm->nf_y, n_elmt, MPIU_REAL, 0, PETSC_COMM_WORLD);
		MPI_Bcast(ibm->nf_z, n_elmt, MPIU_REAL, 0, PETSC_COMM_WORLD);
	}
	else if (rank) {
		MPI_Bcast(&(n_v), 1, MPI_INT, 0, PETSC_COMM_WORLD);
		ibm->n_v = n_v;

		PetscMalloc(n_v*sizeof(PetscReal), &x_bp);
		PetscMalloc(n_v*sizeof(PetscReal), &y_bp);
		PetscMalloc(n_v*sizeof(PetscReal), &z_bp);

		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->x_bp));
		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->y_bp));
		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->z_bp));

		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->x_bp0));
		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->y_bp0));
		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->z_bp0));

		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->x_bp_o));
		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->y_bp_o));
		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->z_bp_o));

		PetscMalloc(n_v*sizeof(Cmpnts), &(ibm->u));
		PetscMalloc(n_v*sizeof(Cmpnts), &(ibm->uold));
/*     ibm->x_bp0 = x_bp;  ibm->y_bp0 = y_bp; ibm->z_bp0 = z_bp; */

		MPI_Bcast(ibm->x_bp0, n_v, MPIU_REAL, 0, PETSC_COMM_WORLD);

		MPI_Bcast(ibm->y_bp0, n_v, MPIU_REAL, 0, PETSC_COMM_WORLD);
		MPI_Bcast(ibm->z_bp0, n_v, MPIU_REAL, 0, PETSC_COMM_WORLD);

		MPI_Bcast(&(n_elmt), 1, MPI_INT, 0, PETSC_COMM_WORLD);
		ibm->n_elmt = n_elmt;

		PetscMalloc(n_elmt*sizeof(PetscInt), &nv1);
		PetscMalloc(n_elmt*sizeof(PetscInt), &nv2);
		PetscMalloc(n_elmt*sizeof(PetscInt), &nv3);

		PetscMalloc(n_elmt*sizeof(PetscReal), &nf_x);
		PetscMalloc(n_elmt*sizeof(PetscReal), &nf_y);
		PetscMalloc(n_elmt*sizeof(PetscReal), &nf_z);

		ibm->nv1 = nv1; ibm->nv2 = nv2; ibm->nv3 = nv3;
		ibm->nf_x = nf_x; ibm->nf_y = nf_y; ibm->nf_z = nf_z;

		MPI_Bcast(ibm->nv1, n_elmt, MPI_INT, 0, PETSC_COMM_WORLD);
		MPI_Bcast(ibm->nv2, n_elmt, MPI_INT, 0, PETSC_COMM_WORLD);
		MPI_Bcast(ibm->nv3, n_elmt, MPI_INT, 0, PETSC_COMM_WORLD);

		MPI_Bcast(ibm->nf_x, n_elmt, MPIU_REAL, 0, PETSC_COMM_WORLD);
		MPI_Bcast(ibm->nf_y, n_elmt, MPIU_REAL, 0, PETSC_COMM_WORLD);
		MPI_Bcast(ibm->nf_z, n_elmt, MPIU_REAL, 0, PETSC_COMM_WORLD);

/*     MPI_Bcast(&(ibm->nv1), n_elmt, MPI_INTEGER, 0, PETSC_COMM_WORLD); */
/*     MPI_Bcast(&(ibm->nv2), n_elmt, MPI_INTEGER, 0, PETSC_COMM_WORLD); */
/*     MPI_Bcast(&(ibm->nv3), n_elmt, MPI_INTEGER, 0, PETSC_COMM_WORLD); */

/*     MPI_Bcast(&(ibm->nf_x), n_elmt, MPIU_REAL, 0, PETSC_COMM_WORLD); */
/*     MPI_Bcast(&(ibm->nf_y), n_elmt, MPIU_REAL, 0, PETSC_COMM_WORLD); */
/*     MPI_Bcast(&(ibm->nf_z), n_elmt, MPIU_REAL, 0, PETSC_COMM_WORLD); */
	}

/*   MPI_Barrier(PETSC_COMM_WORLD); */
	return(0);
}

PetscErrorCode ibm_read_ucd(IBMNodes *ibm)
{
	PetscInt	rank;
	PetscInt	n_v , n_elmt ;
	PetscReal	*x_bp , *y_bp , *z_bp ;
	PetscInt	*nv1 , *nv2 , *nv3 ;
	PetscReal	*nf_x, *nf_y, *nf_z;
	PetscInt	i;
	PetscInt	n1e, n2e, n3e;
	PetscReal	dx12, dy12, dz12, dx13, dy13, dz13;
	PetscReal	t, dr;
	PetscInt 	temp;
	double xt;
	char string[128];
	//MPI_Comm_size(PETSC_COMM_WORLD, &size);
	MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

	if (!rank) { // root processor read in the data
		FILE *fd;
		fd = fopen("ibmdata", "r");
		if (!fd) SETERRQ(1, "Cannot open IBM node file")
		n_v =0;

		if (fd) {
			fgets(string, 128, fd);
			fgets(string, 128, fd);
			fgets(string, 128, fd);

			fscanf(fd, "%i %i %i %i %i\n", &n_v, &n_elmt, &temp, &temp, &temp);

			ibm->n_v = n_v;
			ibm->n_elmt = n_elmt;

			MPI_Bcast(&(ibm->n_v), 1, MPI_INT, 0, PETSC_COMM_WORLD);
			//    PetscPrintf(PETSC_COMM_WORLD, "nv, %d %e \n", n_v, xt);
			PetscMalloc(n_v*sizeof(PetscReal), &x_bp);
			PetscMalloc(n_v*sizeof(PetscReal), &y_bp);
			PetscMalloc(n_v*sizeof(PetscReal), &z_bp);

			PetscMalloc(n_v*sizeof(PetscReal), &(ibm->x_bp));
			PetscMalloc(n_v*sizeof(PetscReal), &(ibm->y_bp));
			PetscMalloc(n_v*sizeof(PetscReal), &(ibm->z_bp));

			PetscMalloc(n_v*sizeof(PetscReal), &(ibm->x_bp_o));
			PetscMalloc(n_v*sizeof(PetscReal), &(ibm->y_bp_o));
			PetscMalloc(n_v*sizeof(PetscReal), &(ibm->z_bp_o));

			PetscMalloc(n_v*sizeof(Cmpnts), &(ibm->u));


			PetscReal cl = 1.;

			PetscOptionsGetReal(PETSC_NULL, "-chact_leng_valve", &cl, PETSC_NULL);

			for (i=0; i<n_v; i++) {
				fscanf(fd, "%i %le %le %le", &temp, &x_bp[i], &y_bp[i], &z_bp[i]);
				x_bp[i] = x_bp[i] / cl;
				y_bp[i] = y_bp[i] / cl;
				z_bp[i] = z_bp[i] / cl;

				ibm->x_bp[i] = x_bp[i];
				ibm->y_bp[i] = y_bp[i];
				ibm->z_bp[i] = z_bp[i];

				ibm->u[i].x = 0.;
				ibm->u[i].y = 0.;
				ibm->u[i].z = 0.;
			}
			ibm->x_bp0 = x_bp; ibm->y_bp0 = y_bp; ibm->z_bp0 = z_bp;

			MPI_Bcast(ibm->x_bp0, n_v, MPIU_REAL, 0, PETSC_COMM_WORLD);
			MPI_Bcast(ibm->y_bp0, n_v, MPIU_REAL, 0, PETSC_COMM_WORLD);
			MPI_Bcast(ibm->z_bp0, n_v, MPIU_REAL, 0, PETSC_COMM_WORLD);

			MPI_Bcast(&(ibm->n_elmt), 1, MPI_INT, 0, PETSC_COMM_WORLD);

			PetscMalloc(n_elmt*sizeof(PetscInt), &nv1);
			PetscMalloc(n_elmt*sizeof(PetscInt), &nv2);
			PetscMalloc(n_elmt*sizeof(PetscInt), &nv3);

			PetscMalloc(n_elmt*sizeof(PetscReal), &nf_x);
			PetscMalloc(n_elmt*sizeof(PetscReal), &nf_y);
			PetscMalloc(n_elmt*sizeof(PetscReal), &nf_z);
			char str[20];
			for (i=0; i<n_elmt; i++) {

				fscanf(fd, "%i %i %s %i %i %i\n", &temp, &temp, str, nv1+i, nv2+i, nv3+i);
				nv1[i] = nv1[i] - 1; nv2[i] = nv2[i]-1; nv3[i] = nv3[i] - 1;

			}
			ibm->nv1 = nv1; ibm->nv2 = nv2; ibm->nv3 = nv3;

			fclose(fd);
		}
		for (i=0; i<n_elmt; i++) {
			n1e = nv1[i]; n2e =nv2[i]; n3e = nv3[i];
			dx12 = x_bp[n2e] - x_bp[n1e];
			dy12 = y_bp[n2e] - y_bp[n1e];
			dz12 = z_bp[n2e] - z_bp[n1e];

			dx13 = x_bp[n3e] - x_bp[n1e];
			dy13 = y_bp[n3e] - y_bp[n1e];
			dz13 = z_bp[n3e] - z_bp[n1e];

			nf_x[i] = dy12 * dz13 - dz12 * dy13;
			nf_y[i] = -dx12 * dz13 + dz12 * dx13;
			nf_z[i] = dx12 * dy13 - dy12 * dx13;

			dr = sqrt(nf_x[i]*nf_x[i] + nf_y[i]*nf_y[i] + nf_z[i]*nf_z[i]);

			nf_x[i] /=dr; nf_y[i]/=dr; nf_z[i]/=dr;
		}

		ibm->nf_x = nf_x; ibm->nf_y = nf_y;  ibm->nf_z = nf_z;

		/*     for (i=0; i<n_elmt; i++) { */
		/*       PetscPrintf(PETSC_COMM_WORLD, "%d %d %d %d\n", i, nv1[i], nv2[i], nv3[i]); */
		/*     } */
		MPI_Bcast(ibm->nv1, n_elmt, MPI_INT, 0, PETSC_COMM_WORLD);
		MPI_Bcast(ibm->nv2, n_elmt, MPI_INT, 0, PETSC_COMM_WORLD);
		MPI_Bcast(ibm->nv3, n_elmt, MPI_INT, 0, PETSC_COMM_WORLD);

		MPI_Bcast(ibm->nf_x, n_elmt, MPIU_REAL, 0, PETSC_COMM_WORLD);
		MPI_Bcast(ibm->nf_y, n_elmt, MPIU_REAL, 0, PETSC_COMM_WORLD);
		MPI_Bcast(ibm->nf_z, n_elmt, MPIU_REAL, 0, PETSC_COMM_WORLD);
	}
	else if (rank) {
		MPI_Bcast(&(n_v), 1, MPI_INT, 0, PETSC_COMM_WORLD);
		ibm->n_v = n_v;

		PetscMalloc(n_v*sizeof(PetscReal), &x_bp);
		PetscMalloc(n_v*sizeof(PetscReal), &y_bp);
		PetscMalloc(n_v*sizeof(PetscReal), &z_bp);

		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->x_bp));
		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->y_bp));
		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->z_bp));

		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->x_bp0));
		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->y_bp0));
		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->z_bp0));

		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->x_bp_o));
		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->y_bp_o));
		PetscMalloc(n_v*sizeof(PetscReal), &(ibm->z_bp_o));

		PetscMalloc(n_v*sizeof(Cmpnts), &(ibm->u));

/*     ibm->x_bp0 = x_bp;  ibm->y_bp0 = y_bp; ibm->z_bp0 = z_bp; */

		MPI_Bcast(ibm->x_bp0, n_v, MPIU_REAL, 0, PETSC_COMM_WORLD);

		MPI_Bcast(ibm->y_bp0, n_v, MPIU_REAL, 0, PETSC_COMM_WORLD);
		MPI_Bcast(ibm->z_bp0, n_v, MPIU_REAL, 0, PETSC_COMM_WORLD);

		for (i=0; i<ibm->n_v; i++) {
			ibm->x_bp[i] = ibm->x_bp0[i];
			ibm->y_bp[i] = ibm->y_bp0[i];
			ibm->z_bp[i] = ibm->z_bp0[i];

			ibm->u[i].x = 0.;
			ibm->u[i].y = 0.;
			ibm->u[i].z = 0.;
		}
		MPI_Bcast(&(n_elmt), 1, MPI_INT, 0, PETSC_COMM_WORLD);
		ibm->n_elmt = n_elmt;

		PetscMalloc(n_elmt*sizeof(PetscInt), &nv1);
		PetscMalloc(n_elmt*sizeof(PetscInt), &nv2);
		PetscMalloc(n_elmt*sizeof(PetscInt), &nv3);

		PetscMalloc(n_elmt*sizeof(PetscReal), &nf_x);
		PetscMalloc(n_elmt*sizeof(PetscReal), &nf_y);
		PetscMalloc(n_elmt*sizeof(PetscReal), &nf_z);

		ibm->nv1 = nv1; ibm->nv2 = nv2; ibm->nv3 = nv3;
		ibm->nf_x = nf_x; ibm->nf_y = nf_y; ibm->nf_z = nf_z;

		MPI_Bcast(ibm->nv1, n_elmt, MPI_INT, 0, PETSC_COMM_WORLD);
		MPI_Bcast(ibm->nv2, n_elmt, MPI_INT, 0, PETSC_COMM_WORLD);
		MPI_Bcast(ibm->nv3, n_elmt, MPI_INT, 0, PETSC_COMM_WORLD);

		MPI_Bcast(ibm->nf_x, n_elmt, MPIU_REAL, 0, PETSC_COMM_WORLD);
		MPI_Bcast(ibm->nf_y, n_elmt, MPIU_REAL, 0, PETSC_COMM_WORLD);
		MPI_Bcast(ibm->nf_z, n_elmt, MPIU_REAL, 0, PETSC_COMM_WORLD);

/*     MPI_Bcast(&(ibm->nv1), n_elmt, MPI_INTEGER, 0, PETSC_COMM_WORLD); */
/*     MPI_Bcast(&(ibm->nv2), n_elmt, MPI_INTEGER, 0, PETSC_COMM_WORLD); */
/*     MPI_Bcast(&(ibm->nv3), n_elmt, MPI_INTEGER, 0, PETSC_COMM_WORLD); */

/*     MPI_Bcast(&(ibm->nf_x), n_elmt, MPIU_REAL, 0, PETSC_COMM_WORLD); */
/*     MPI_Bcast(&(ibm->nf_y), n_elmt, MPIU_REAL, 0, PETSC_COMM_WORLD); */
/*     MPI_Bcast(&(ibm->nf_z), n_elmt, MPIU_REAL, 0, PETSC_COMM_WORLD); */
	}

/*   MPI_Barrier(PETSC_COMM_WORLD); */
	return(0);
}

PetscErrorCode Combine_Elmt(IBMNodes *ibm, IBMNodes *ibm0, IBMNodes *ibm1)
{

	PetscInt i;

	ibm->n_v = ibm0->n_v + ibm1->n_v;
	ibm->n_elmt = ibm0->n_elmt + ibm1->n_elmt;

	PetscInt n_v = ibm->n_v, n_elmt = ibm->n_elmt;

	for (i=0; i<ibm0->n_v; i++) {
		ibm->x_bp[i] = ibm0->x_bp[i];
		ibm->y_bp[i] = ibm0->y_bp[i];
		ibm->z_bp[i] = ibm0->z_bp[i];

		ibm->u[i] = ibm0->u[i];
		ibm->uold[i] = ibm0->uold[i];
		//    ibm->u[i].x = 0.;
/*     PetscPrintf(PETSC_COMM_WORLD, "Vel %e %e %e\n", ibm->u[i].x, ibm->u[i].y, ibm->u[i].z); */
	}
	for (i=0; i<ibm0->n_elmt; i++) {
		ibm->nv1[i] = ibm0->nv1[i];
		ibm->nv2[i] = ibm0->nv2[i];
		ibm->nv3[i] = ibm0->nv3[i];

		ibm->nf_x[i] = ibm0->nf_x[i];
		ibm->nf_y[i] = ibm0->nf_y[i];
		ibm->nf_z[i] = ibm0->nf_z[i];
	}

	for (i=ibm0->n_v; i<n_v; i++) {
		ibm->x_bp[i] = ibm1->x_bp[i-ibm0->n_v];
		ibm->y_bp[i] = ibm1->y_bp[i-ibm0->n_v];
		ibm->z_bp[i] = ibm1->z_bp[i-ibm0->n_v];
		ibm->u[i].x = 0.;
		ibm->u[i].y = 0.;
		ibm->u[i].z = 0.;
	}

	for (i=ibm0->n_elmt; i<n_elmt; i++) {
		ibm->nv1[i] = ibm1->nv1[i-ibm0->n_elmt] + ibm0->n_v;
		ibm->nv2[i] = ibm1->nv2[i-ibm0->n_elmt] + ibm0->n_v;
		ibm->nv3[i] = ibm1->nv3[i-ibm0->n_elmt] + ibm0->n_v;

		ibm->nf_x[i] = ibm1->nf_x[i-ibm0->n_elmt];
		ibm->nf_y[i] = ibm1->nf_y[i-ibm0->n_elmt];
		ibm->nf_z[i] = ibm1->nf_z[i-ibm0->n_elmt];
	}

	PetscInt rank;
	MPI_Comm_rank(PETSC_COMM_WORLD, &rank);
	if (!rank) {
		if (ti == ti) {
			FILE *f;
			char filen[80];
			sprintf(filen, "surface%05d.dat",ti);
			f = fopen(filen, "w");
			PetscFPrintf(PETSC_COMM_WORLD, f, "Variables=x,y,z\n");
			PetscFPrintf(PETSC_COMM_WORLD, f, "ZONE T='TRIANGLES', N=%d, E=%d, F=FEPOINT, ET=TRIANGLE\n", n_v, 1670-96);
			for (i=0; i<n_v; i++) {

	/*    ibm->x_bp[i] = ibm->x_bp0[i];
				ibm->y_bp[i] = ibm->y_bp0[i];
				ibm->z_bp[i] = ibm->z_bp0[i] + z0;*/
				PetscFPrintf(PETSC_COMM_WORLD, f, "%e %e %e\n", ibm->x_bp[i], ibm->y_bp[i], ibm->z_bp[i]);
			}
			for (i=96; i<n_elmt; i++) {
				if (fabs(ibm->nf_z[i]) > 0.5 ||
						(fabs(ibm->nf_z[i]) < 0.5 &&
						 (ibm->x_bp[ibm->nv1[i]] * ibm->x_bp[ibm->nv1[i]] +
							ibm->y_bp[ibm->nv1[i]] * ibm->y_bp[ibm->nv1[i]]) < 0.44*0.44)) {
					PetscFPrintf(PETSC_COMM_WORLD, f, "%d %d %d\n", ibm->nv1[i]+1, ibm->nv2[i]+1, ibm->nv3[i]+1);
				}
			}
			fclose(f);

			sprintf(filen, "leaflet%05d.dat",ti);
			f = fopen(filen, "w");
			PetscFPrintf(PETSC_COMM_WORLD, f, "Variables=x,y,z\n");
			PetscFPrintf(PETSC_COMM_WORLD, f, "ZONE T='TRIANGLES', N=%d, E=%d, F=FEPOINT, ET=TRIANGLE\n", n_v, 96);
			for (i=0; i<n_v; i++) {

	/*    ibm->x_bp[i] = ibm->x_bp0[i];
				ibm->y_bp[i] = ibm->y_bp0[i];
				ibm->z_bp[i] = ibm->z_bp0[i] + z0;*/
				PetscFPrintf(PETSC_COMM_WORLD, f, "%e %e %e\n", ibm->x_bp[i], ibm->y_bp[i], ibm->z_bp[i]);
			}
			for (i=0; i<96; i++) {
				if (fabs(ibm->nf_z[i]) > 0.5 ||
						(fabs(ibm->nf_z[i]) < 0.5 &&
						 (ibm->x_bp[ibm->nv1[i]] * ibm->x_bp[ibm->nv1[i]] +
							ibm->y_bp[ibm->nv1[i]] * ibm->y_bp[ibm->nv1[i]]) < 0.44*0.44)) {
					PetscFPrintf(PETSC_COMM_WORLD, f, "%d %d %d\n", ibm->nv1[i]+1, ibm->nv2[i]+1, ibm->nv3[i]+1);
				}
			}
			fclose(f);

		}
	}

	return 0;
}

PetscErrorCode Elmt_Move(IBMNodes *ibm, UserCtx *user)
{
	PetscInt n_v = ibm->n_v, n_elmt = ibm->n_elmt;

	PetscReal rcx = -0.122, rcz = -0.32, z0 = 4.52;
	rcx = -0.09450115; rcz = -0.3141615; z0 = 4.47;
	PetscReal dz;
	dz = -0.031;
	rcz = rcz-dz;
	rcx = rcx - dz * sin(10./180.*3.1415926);
	PetscReal temp;
	PetscInt i;

	PetscInt n1e, n2e, n3e;
	PetscReal dx12, dy12, dz12, dx13, dy13, dz13, dr;
	for (i=0; i<n_v; i++) {
		ibm->x_bp_o[i] = ibm->x_bp[i];
		ibm->y_bp_o[i] = ibm->y_bp[i];
		ibm->z_bp_o[i] = ibm->z_bp[i];
	}

	angle =-angle * 3.1415926/180.;
	//angle = 0;
	for (i=0; i<n_v/2; i++) {
		ibm->x_bp[i] = (ibm->x_bp0[i] -0.01- rcx) * cos(angle) - (ibm->z_bp0[i] - rcz) * sin(angle) + rcx;
		ibm->y_bp[i] = ibm->y_bp0[i];
		ibm->z_bp[i] = (ibm->x_bp0[i] -0.01- rcx) * sin(angle) + (ibm->z_bp0[i] - rcz) * cos(angle) + z0 + rcz;

	}
	rcx = -rcx;
	for (i=n_v/2; i<n_v; i++) {
		ibm->x_bp[i] = (ibm->x_bp0[i] +0.01- rcx) * cos(-angle) - (ibm->z_bp0[i] - rcz) * sin(-angle) + rcx;
		ibm->y_bp[i] = ibm->y_bp0[i];
		ibm->z_bp[i] = (ibm->x_bp0[i] +0.01- rcx) * sin(-angle) + (ibm->z_bp0[i] - rcz) * cos(-angle) + z0 + rcz;
	}

	/* Rotate 90 degree */
	for (i=0; i<n_v; i++) {
		temp = ibm->y_bp[i];
		ibm->y_bp[i] = ibm->x_bp[i];
		ibm->x_bp[i] = temp;
	}
	for (i=0; i<n_elmt; i++) {
			n1e = ibm->nv1[i]; n2e =ibm->nv2[i]; n3e =ibm->nv3[i];
			dx12 = ibm->x_bp[n2e] - ibm->x_bp[n1e];
			dy12 = ibm->y_bp[n2e] - ibm->y_bp[n1e];
			dz12 = ibm->z_bp[n2e] - ibm->z_bp[n1e];

			dx13 = ibm->x_bp[n3e] - ibm->x_bp[n1e];
			dy13 = ibm->y_bp[n3e] - ibm->y_bp[n1e];
			dz13 = ibm->z_bp[n3e] - ibm->z_bp[n1e];

			ibm->nf_x[i] = dy12 * dz13 - dz12 * dy13;
			ibm->nf_y[i] = -dx12 * dz13 + dz12 * dx13;
			ibm->nf_z[i] = dx12 * dy13 - dy12 * dx13;

			dr = sqrt(ibm->nf_x[i]*ibm->nf_x[i] +
								ibm->nf_y[i]*ibm->nf_y[i] +
								ibm->nf_z[i]*ibm->nf_z[i]);

			ibm->nf_x[i] /=dr; ibm->nf_y[i]/=dr; ibm->nf_z[i]/=dr;

/*       PetscPrintf(PETSC_COMM_WORLD, "NFZ %d %d %d %d %e\n", i, ibm->nv1[i], ibm->nv2[i], ibm->nv3[i], ibm->nf_z[i]); */
			//      PetscPrintf(PETSC_COMM_WORLD, "%le %le %le %le %le %le\n", x_bp[n1e], y_bp[n1e], ibm->x_bp0[n1e], ibm->y_bp0[n1e], x_bp[n3e], y_bp[n3e]);

	}
	if (ti>0) {
		for (i=0; i<n_v; i++) {
			//      ibm->uold[i] = ibm->u[i];

			ibm->u[i].x = (ibm->x_bp[i] - ibm->x_bp_o[i]) / user->dt;
			ibm->u[i].y = (ibm->y_bp[i] - ibm->y_bp_o[i]) / user->dt;
			ibm->u[i].z = (ibm->z_bp[i] - ibm->z_bp_o[i]) / user->dt;
		}
	}
	else {
		for (i=0; i<n_v; i++) {
			ibm->u[i].x = 0.;
			ibm->u[i].y = 0.;
			ibm->u[i].z = 0.;
		}
	}
	return 0;
}

PetscErrorCode Elmt_Move1(IBMNodes *ibm, UserCtx *user)
{
	PetscInt n_v = ibm->n_v, n_elmt = ibm->n_elmt;

	PetscReal rcx = -0.122, rcz = -0.32, z0 = 4.52;
	rcx = -0.09450115; rcz = -0.3141615; z0 = 4.47;
	PetscReal dz;
	dz = -0.031;
	rcz = rcz-dz;
	rcx = rcx - dz * sin(10./180.*3.1415926);
	PetscReal temp;
	PetscInt i;

	PetscInt n1e, n2e, n3e;
	PetscReal dx12, dy12, dz12, dx13, dy13, dz13, dr;
	for (i=0; i<n_v; i++) {
		ibm->x_bp_o[i] = ibm->x_bp[i];
		ibm->y_bp_o[i] = ibm->y_bp[i];
		ibm->z_bp_o[i] = ibm->z_bp[i];
	}

	angle =-angle * 3.1415926/180.;
	//angle = 0;
	for (i=0; i<n_v/2; i++) {
		ibm->x_bp[i] = (ibm->x_bp0[i] -0.01- rcx) * cos(angle) - (ibm->z_bp0[i] - rcz) * sin(angle) + rcx;
		ibm->y_bp[i] = ibm->y_bp0[i];
		ibm->z_bp[i] = (ibm->x_bp0[i] -0.01- rcx) * sin(angle) + (ibm->z_bp0[i] - rcz) * cos(angle) + z0 + rcz;

	}
	rcx = -rcx;
	for (i=n_v/2; i<n_v; i++) {
		ibm->x_bp[i] = (ibm->x_bp0[i] +0.01- rcx) * cos(-angle) - (ibm->z_bp0[i] - rcz) * sin(-angle) + rcx;
		ibm->y_bp[i] = ibm->y_bp0[i];
		ibm->z_bp[i] = (ibm->x_bp0[i] +0.01- rcx) * sin(-angle) + (ibm->z_bp0[i] - rcz) * cos(-angle) + z0 + rcz;
	}

	/* Rotate 90 degree */
	for (i=0; i<n_v; i++) {
		temp = ibm->y_bp[i];
		ibm->y_bp[i] = ibm->x_bp[i];
		ibm->x_bp[i] = temp;
	}
	for (i=0; i<n_elmt; i++) {
			n1e = ibm->nv1[i]; n2e =ibm->nv2[i]; n3e =ibm->nv3[i];
			dx12 = ibm->x_bp[n2e] - ibm->x_bp[n1e];
			dy12 = ibm->y_bp[n2e] - ibm->y_bp[n1e];
			dz12 = ibm->z_bp[n2e] - ibm->z_bp[n1e];

			dx13 = ibm->x_bp[n3e] - ibm->x_bp[n1e];
			dy13 = ibm->y_bp[n3e] - ibm->y_bp[n1e];
			dz13 = ibm->z_bp[n3e] - ibm->z_bp[n1e];

			ibm->nf_x[i] = dy12 * dz13 - dz12 * dy13;
			ibm->nf_y[i] = -dx12 * dz13 + dz12 * dx13;
			ibm->nf_z[i] = dx12 * dy13 - dy12 * dx13;

			dr = sqrt(ibm->nf_x[i]*ibm->nf_x[i] +
								ibm->nf_y[i]*ibm->nf_y[i] +
								ibm->nf_z[i]*ibm->nf_z[i]);

			ibm->nf_x[i] /=dr; ibm->nf_y[i]/=dr; ibm->nf_z[i]/=dr;

/*       PetscPrintf(PETSC_COMM_WORLD, "NFZ %d %d %d %d %e\n", i, ibm->nv1[i], ibm->nv2[i], ibm->nv3[i], ibm->nf_z[i]); */
			//      PetscPrintf(PETSC_COMM_WORLD, "%le %le %le %le %le %le\n", x_bp[n1e], y_bp[n1e], ibm->x_bp0[n1e], ibm->y_bp0[n1e], x_bp[n3e], y_bp[n3e]);

	}
	if (ti>0) {
		for (i=0; i<n_v; i++) {
			ibm->uold[i] = ibm->u[i];

			ibm->u[i].x = (ibm->x_bp[i] - ibm->x_bp_o[i]) / user->dt;
			ibm->u[i].y = (ibm->y_bp[i] - ibm->y_bp_o[i]) / user->dt;
			ibm->u[i].z = (ibm->z_bp[i] - ibm->z_bp_o[i]) / user->dt;
		}
	}
	else {
		for (i=0; i<n_v; i++) {
			ibm->u[i].x = 0.;
			ibm->u[i].y = 0.;
			ibm->u[i].z = 0.;
		}
	}
	return 0;
}


/*****************************************************************/
#ifdef MAX
#undef MAX
#endif

#define MAX(a, b) ((a)>(b)?(a):(b))

#define n 3

static double hypot2(double x, double y) {
	return sqrt(x*x+y*y);
}

// Symmetric Householder reduction to tridiagonal form.

static void tred2(double V[n][n], double d[n], double e[n]) {

//  This is derived from the Algol procedures tred2 by
//  Bowdler, Martin, Reinsch, and Wilkinson, Handbook for
//  Auto. Comp., Vol.ii-Linear Algebra, and the corresponding
//  Fortran subroutine in EISPACK.

	for (int j = 0; j < n; j++) {
		d[j] = V[n-1][j];
	}

	// Householder reduction to tridiagonal form.

	for (int i = n-1; i > 0; i--) {

		// Scale to avoid under/overflow.

		double scale = 0.0;
		double h = 0.0;
		for (int k = 0; k < i; k++) {
			scale = scale + fabs(d[k]);
		}
		if (scale == 0.0) {
			e[i] = d[i-1];
			for (int j = 0; j < i; j++) {
				d[j] = V[i-1][j];
				V[i][j] = 0.0;
				V[j][i] = 0.0;
			}
		} else {

			// Generate Householder vector.

			for (int k = 0; k < i; k++) {
				d[k] /= scale;
				h += d[k] * d[k];
			}
			double f = d[i-1];
			double g = sqrt(h);
			if (f > 0) {
				g = -g;
			}
			e[i] = scale * g;
			h = h - f * g;
			d[i-1] = f - g;
			for (int j = 0; j < i; j++) {
				e[j] = 0.0;
			}

			// Apply similarity transformation to remaining columns.

			for (int j = 0; j < i; j++) {
				f = d[j];
				V[j][i] = f;
				g = e[j] + V[j][j] * f;
				for (int k = j+1; k <= i-1; k++) {
					g += V[k][j] * d[k];
					e[k] += V[k][j] * f;
				}
				e[j] = g;
			}
			f = 0.0;
			for (int j = 0; j < i; j++) {
				e[j] /= h;
				f += e[j] * d[j];
			}
			double hh = f / (h + h);
			for (int j = 0; j < i; j++) {
				e[j] -= hh * d[j];
			}
			for (int j = 0; j < i; j++) {
				f = d[j];
				g = e[j];
				for (int k = j; k <= i-1; k++) {
					V[k][j] -= (f * e[k] + g * d[k]);
				V[i][j] = 0.0;
				}
			}
		}
		d[i] = h;
	}
// Accumulate tranitmations.

	for (int i = 0; i < n-1; i++) {
		V[n-1][i] = V[i][i];
		V[i][i] = 1.0;
		double h = d[i+1];
		if (h != 0.0) {
			for (int k = 0; k <= i; k++) {
				d[k] = V[k][i+1] / h;
			}
			for (int j = 0; j <= i; j++) {
				double g = 0.0;
				for (int k = 0; k <= i; k++) {
					g += V[k][i+1] * V[k][j];
				}
				for (int k = 0; k <= i; k++) {
					V[k][j] -= g * d[k];
				}
			}
		}
		for (int k = 0; k <= i; k++) {
			V[k][i+1] = 0.0;
		}
	}
	for (int j = 0; j < n; j++) {
		d[j] = V[n-1][j];
		V[n-1][j] = 0.0;
	}
	V[n-1][n-1] = 1.0;
	e[0] = 0.0;
}

// Symmetric tridiagonal QL algorithm.

static void tql2(double V[n][n], double d[n], double e[n]) {

//  This is derived from the Algol procedures tql2, by
//  Bowdler, Martin, Reinsch, and Wilkinson, Handbook for
//  Auto. Comp., Vol.ii-Linear Algebra, and the corresponding
//  Fortran subroutine in EISPACK.

	for (int i = 1; i < n; i++) {
		e[i-1] = e[i];
	}
	e[n-1] = 0.0;

	double f = 0.0;
	double tst1 = 0.0;
	double eps = pow(2.0,-52.0);
	for (int l = 0; l < n; l++) {

		// Find small subdiagonal element

		tst1 = MAX(tst1,fabs(d[l]) + fabs(e[l]));
		int m = l;
		while (m < n) {
			if (fabs(e[m]) <= eps*tst1) {
				break;
			}
			m++;
		}

		// If m == l, d[l] is an eigenvalue,
		// otherwise, iterate.

		if (m > l) {
			int iter = 0;
			do {
				iter = iter + 1;  // (Could check iteration count here.)

				// Compute implicit shift

				double g = d[l];
				double p = (d[l+1] - g) / (2.0 * e[l]);
				double r = hypot2(p,1.0);
				if (p < 0) {
					r = -r;
				}
				d[l] = e[l] / (p + r);
				d[l+1] = e[l] * (p + r);
				double dl1 = d[l+1];
				double h = g - d[l];
				for (int i = l+2; i < n; i++) {
					d[i] -= h;
				}
				f = f + h;

				// Implicit QL transformation.

				p = d[m];
				double c = 1.0;
				double c2 = c;
				double c3 = c;
				double el1 = e[l+1];
				double s = 0.0;
				double s2 = 0.0;
				for (int i = m-1; i >= l; i--) {
					c3 = c2;
					c2 = c;
					s2 = s;
					g = c * e[i];
					h = c * p;
					r = hypot2(p,e[i]);
					e[i+1] = s * r;
					s = e[i] / r;
					c = p / r;
					p = c * d[i] - s * g;
					d[i+1] = h + s * (c * g + s * d[i]);

					// Accumulate transformation.

					for (int k = 0; k < n; k++) {
						h = V[k][i+1];
						V[k][i+1] = s * V[k][i] + c * h;
						V[k][i] = c * V[k][i] - s * h;
					}
				}
				p = -s * s2 * c3 * el1 * e[l] / dl1;
				e[l] = s * p;
				d[l] = c * p;

				// Check for convergence.

			} while (fabs(e[l]) > eps*tst1);
		}
		d[l] = d[l] + f;
		e[l] = 0.0;
	}

	// Sort eigenvalues and corresponding vectors.

	for (int i = 0; i < n-1; i++) {
		int k = i;
		double p = d[i];
		for (int j = i+1; j < n; j++) {
			if (d[j] < p) {
				k = j;
				p = d[j];
			}
		}
		if (k != i) {
			d[k] = d[i];
			d[i] = p;
			for (int j = 0; j < n; j++) {
				p = V[j][i];
				V[j][i] = V[j][k];
				V[j][k] = p;
			}
		}
	}
}

void eigen_decomposition(double A[n][n], double V[n][n], double d[n]) {
        double e[n];
        for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                        V[i][j] = A[i][j];
                }
        }
        tred2(V, d, e);
        tql2(V, d, e);
}

