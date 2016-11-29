
#include "binvoxToPcl.h"
#include "assign_confidence.h"

#include "ConstConf.h"

#include "voxelize.h"

#include "narrowBand.h"

#include "quadprog.h"

using namespace std;

typedef unsigned char byte;

int main(int argc, char **argv){
    bool USING_FEATURES=false;

    if(argc != 3){
        cout <<"Usage: Optimize Mesh with feature detection <binvox completed filename> <pcd observed filename>" << endl << endl;
        exit(1);
    }

    /* Read data from files into point clouds */
    pcl::PointCloud<pcl::PointXYZ>::Ptr observeCloud (new pcl::PointCloud<pcl::PointXYZ>());
    if (pcl::io::loadPCDFile<pcl::PointXYZ> (argv[2], *observeCloud) == -1){ //* load the file
        PCL_ERROR ("Couldn't read pcd file \n");
        exit(1);
    }
    int res = getResolutionFactor(argv[1], observeCloud);
    //read in data from binvox file+adjust resolution
    pcl::PointCloud<pcl::PointXYZ>::Ptr predictCloud = binvoxToPCL(argv[1], res);


    /* Combine into pcl_conf with confidences */
    Confidencor *confidence_assigner = new ConstConf(1.0); //<--- change confidencor function here
    //assign full confidence to observeCloud
    pcl::PointCloud<pcl::InterestPoint>::Ptr confPCL=full_confidence(observeCloud);
    //asign confidence to everything
    assign_confidence(confPCL, predictCloud, confidence_assigner);


    /* Create voxel grid from point clouds */
    voxelized_dataPtr data = voxelizeData(confPCL); //<--test different resolutions
    //create grids
    gridPtr grid_cloud = createGrid(data->filtered_cloud, data->grid_data, res);
    gridPtr volume = getBinaryVolume(grid_cloud);


    /* Perform feature detection */
    vector<int> surface = getSurface(volume);
    gridPtr surfaceMap = getIndexMap(volume, surface);
    vector<Eigen::Vector3f> normals = getSurfaceNormals(volume, surface);
    gridPtr featureMap = getFeatureMap(volume, surfaceMap, normals, 0.85);


    /* Perform smoothing */
    //get imbedding function
    gridPtr F = optimize(volume, featureMap, USING_FEATURES, false);

    F->visualize();

    return 1;
}
