#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include "curvature.h"
#include<iostream>

using namespace OpenMesh;
using namespace Eigen;
using namespace std;

void computeCurvature(Mesh &mesh, OpenMesh::VPropHandleT<CurvatureInfo> &curvature) {
	int size = 0;
	for (Mesh::VertexIter v_it = mesh.vertices_begin(); v_it != mesh.vertices_end() && size <= 10; ++v_it) {
		// WRITE CODE HERE TO COMPUTE THE CURVATURE AT THE CURRENT VERTEX ----------------------------------------------
		double area = 0.0;
		size += 1;
		const Mesh::VertexHandle vi = (*v_it);
		const Vec3f normal = mesh.normal(vi);
		const Vector3d n(normal[0], normal[1], normal[2]); // example of converting to Eigen's vector class for easier math
		const Mesh::Point& P = mesh.point(v_it);
		const Vector3d coor_i(P[0], P[1], P[2]);
		Matrix3d app_M = Matrix3d::Zero();


		for (Mesh::VertexOHalfedgeIter vf_it = mesh.voh_iter(vi); vf_it.is_valid(); ++vf_it)// find the neighbohood point J
		{

			Vec3f point_j = mesh.point(mesh.to_vertex_handle(vf_it.handle()));
			Vector3d coor_j(point_j[0], point_j[1], point_j[2]);
			//cout << coor_i << endl;
			//cout << coor_j << endl;
			Vector3d vji = coor_i - coor_j;

			Vector3d T_ij = (Matrix3d::Identity() - n * n.transpose())*vji / ((Matrix3d::Identity() - n * n.transpose())*vji).norm();

			//cout << T_ij << endl;
			double K_ij = 2 * n.transpose().dot((vji)) / vji.dot(vji);

			//cout << K_ij << endl;
			//calculate the weight w_{ij}

			double wij = mesh.calc_sector_area(vf_it.handle()) + mesh.calc_sector_area(mesh.opposite_halfedge_handle(vf_it.handle()));
			area += wij;

			app_M += wij * K_ij*T_ij*T_ij.transpose();


		}

		app_M /= area;
		//cout << app_M << endl;
		SelfAdjointEigenSolver<Matrix3d> solver(app_M);
		double eigen1 = solver.eigenvalues()(0);
		double eigen2 = solver.eigenvalues()(1);

		Vector3d T11 = solver.eigenvectors().col(0);
		Vector3d T22 = solver.eigenvectors().col(1);


		double m11 = T11.transpose()*app_M*T11;
		double m22 = T22.transpose()*app_M*T22;







		// In the end you need to fill in this struct

		CurvatureInfo info;
		info.curvatures[0] = 3 * m11 - m22;
		info.curvatures[1] = 3 * m22 - m11;
		info.directions[0] = Vec3f(T11(0), T11(1), T11(2));
		info.directions[1] = Vec3f(T22(0), T22(1), T22(2));

		if (fabs(info.curvatures[0]) > fabs(info.curvatures[1])) {
			double exchange = info.curvatures[0];
			info.curvatures[0] = info.curvatures[1];
			info.curvatures[1] = exchange;

			Vec3f exchange2 = info.directions[0];
			info.directions[0] = info.directions[1];
			info.directions[1] = exchange2;
		}
		// -------------------------------------------------------------------------------------------------------------
	}
}
