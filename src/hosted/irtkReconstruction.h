
#include "common.h"
#include "../utils.h"

#include <irtkImage.h>
#include <irtkTransformation.h>
#include <irtkGaussianBlurring.h>

#include <ebbrt/EbbAllocator.h>
#include <ebbrt/Future.h>
#include <ebbrt/Message.h>
#include <ebbrt/IOBuf.h>
#include <ebbrt/UniqueIOBuf.h>
#include <ebbrt/StaticIOBuf.h>

using namespace ebbrt;

class irtkReconstruction : public ebbrt::Messagable<irtkReconstruction>, public irtkObject {

  private:
    // Ebb creation parameters 
    std::unordered_map<uint32_t, ebbrt::Promise<void>> _promise_map;
    std::mutex _m;
    uint32_t _id{0};

    // EbbRT-related parameters
    std::vector<ebbrt::Messenger::NetworkId> _nids;
    ebbrt::Promise<void> _backendsAllocated;

    // Input parameters
    string _outputName;  // Not used
    string _maskName; // Not used
    string _referenceVolumeName; // Not used
    string _logId; // Not used
    string _tFolder; // Not used
    string _sFolder; // Not used

    vector<string> _inputStacks; // Not used
    vector<string> _inputTransformations; // Not used
    vector<double> _thickness; // Not used
    vector<int> _packages; // Not used
    vector<int> _forceExcluded; // Not used
    vector<int> _devicesToUse; // Not used

    int _iterations;  
    int _levels; // Not used
    int _recIterationsFirst; 
    int _recIterationsLast; 
    int _numThreads; // Not used
    int _numBackendNodes; 
    int _numFrontendCPUs; // Not used

    unsigned int _numInputStacksTuner; // Not used
    unsigned int _T1PackageSize; // Not used
    unsigned int _numDevicesToUse; // Not used

    double _sigma; // Not used
    double _resolution; // Not used
    double _averageValue; // Not used
    double _delta; 
    double _lambda; 
    double _lastIterLambda; // Not used
    double _smoothMask; // Not used
    double _lowIntensityCutoff; // Not used

    bool _globalBiasCorrection; // Not used
    bool _intensityMatching; // Not used
    bool _debug; 
    bool _noLog; 
    bool _useCPU; 
    bool _useCPUReg; // Not used
    bool _useAutoTemplate; // Not used
    bool _useSINCPSF; // Not used
    bool _disableBiasCorr; // Not used

    // Internal parameters
    irtkRealImage _externalRegistrationTargetImage;
    irtkRealImage _reconstructed;
    irtkRealImage _mask;

    vector<double> _scaleCPU;
    vector<double> _sliceWeightCPU;
    vector<double> _slicePotential;

    vector<float> _stackFactor;

    vector<int> _stackIndex;
    vector<int> _sliceInsideCPU;

    vector<irtkRigidTransformation> _transformations;

    vector<irtkRealImage> _slices;
    vector<irtkRealImage> _simulatedSlices;
    vector<irtkRealImage> _simulatedInside;
    vector<irtkRealImage> _simulatedWeights;
    vector<irtkRealImage> _weights;
    vector<irtkRealImage> _bias;

    vector<SLICECOEFFS> _volcoeffs;

    int _directions[13][3];

    int _sigmaBias;
    int _reconRecv;
    int _tsigma;
    int _tmix;
    int _tnum;
    int _totalBytes;
    int _received;


    double _qualityFactor;
    double _step; 
    double _sigmaSCPU;
    double _sigmaS2CPU;
    double _mixSCPU;
    double _mixCPU;
    double _alpha;
    double _tmin;
    double _tmax;
    double _maxIntensity;
    double _minIntensity;

    bool _templateCreated;
    bool _haveMask;
    bool _adaptive;

    ebbrt::Promise<int> _future;

  public:

    // Constructor
    irtkReconstruction(ebbrt::EbbId ebbid);
    
    // Ebb creation functions
    static ebbrt::EbbRef<irtkReconstruction>
      Create(ebbrt::EbbId id = ebbrt::ebb_allocator->Allocate());

    static irtkReconstruction& HandleFault(ebbrt::EbbId id);

    ebbrt::Future<void> Ping(ebbrt::Messenger::NetworkId nid);

    // EbbRT-related functions
    void ReceiveMessage(ebbrt::Messenger::NetworkId nid,
        std::unique_ptr<ebbrt::IOBuf>&& buffer);

    // Node allocation functions
    void AddNid(ebbrt::Messenger::NetworkId nid);

    ebbrt::Future<void> WaitPool();

    // Reconstruction functions
    void SetParameters(arguments args);

    void SetDefaultParameters();

    void SetSmoothingParameters();

    irtkRealImage CreateMask(irtkRealImage image);

    void TransformMask(irtkRealImage& image,
        irtkRealImage& mask,
        irtkRigidTransformation& transformation);

    void CropImage(irtkRealImage& image,
        irtkRealImage& mask);

    void InvertStackTransformations(
        vector<irtkRigidTransformation>& stack_transformations);

    double CreateTemplate(irtkRealImage stack,
        double resolution = 0);

    irtkRealImage GetMask();

    void SetMask(irtkRealImage * mask, double sigma, double threshold = 0.5);

    void StackRegistrations(vector<irtkRealImage>& stacks,
        vector<irtkRigidTransformation>& stack_transformations,
        int templateNumber, bool useExternalTarget = false);

    irtkRealImage CreateAverage(vector<irtkRealImage>& stacks,
        vector<irtkRigidTransformation>& stack_transformations);

    void MatchStackIntensitiesWithMasking(vector<irtkRealImage>& stacks,
        vector<irtkRigidTransformation>& stack_transformations,
        double averageValue,
        bool together = false);

    void CreateSlicesAndTransformations(vector<irtkRealImage>& stacks,
        vector<irtkRigidTransformation>& stack_transformations,
        vector<double>& thickness,
        const vector<irtkRealImage> &probability_maps = vector<irtkRealImage>());

    void MaskSlices();

    void ReadTransformation(char* folder);

    void InitializeEM();

    void InitializeEMValues();

    struct reconstructionParameters CreateReconstructionParameters(int start, int end);

    // CoeffInit() function
    struct coeffInitParameters createCoeffInitParameters();

    void CoeffInit(int iteration);

    //GaussianReconstruction() function
    void GaussianReconstruction();

    void ReturnFrom();

    void Execute();

    // Auxiliary serialize functions
    unique_ptr<ebbrt::MutUniqueIOBuf> SerializeSlices();

    unique_ptr<ebbrt::MutUniqueIOBuf> SerializeReconstructed();

    unique_ptr<ebbrt::MutUniqueIOBuf> SerializeMask();

    unique_ptr<ebbrt::MutUniqueIOBuf> SerializeTransformations();

    /*
    unique_ptr<ebbrt::MutUniqueIOBuf> SerializeImageAttr(irtkRealImage ri);

    unique_ptr<ebbrt::MutUniqueIOBuf> SerializeImageI2W(irtkRealImage& ri);

    unique_ptr<ebbrt::MutUniqueIOBuf> SerializeImageW2I(irtkRealImage& ri);

    unique_ptr<ebbrt::MutUniqueIOBuf> SerializeSlices(irtkRealImage& ri);

    unique_ptr<ebbrt::MutUniqueIOBuf> SerializeRigidTrans(
        irtkRigidTransformation& rt);
    */

    // Static Reconstruction functions
    static void ResetOrigin(irtkGreyImage &image, 
        irtkRigidTransformation& transformation);

    // For debugging purposes
    inline double SumImage(irtkRealImage img);

    inline void PrintImageSums();

    inline void PrintVectorSums(vector<irtkRealImage> images, string name);

    inline void PrintAttributeVectorSums();
};

inline double irtkReconstruction::SumImage(irtkRealImage img) {
  float sum = 0.0;
  irtkRealPixel *ap = img.GetPointerToVoxels();

  for (int j = 0; j < img.GetNumberOfVoxels(); j++) {
      sum += (float)*ap;
    ap++;
  }
  return (double)sum;
}

inline void irtkReconstruction::PrintImageSums() {
  /*
  cout << "_externalRegistrationTargetImage: " 
       << SumImage(_externalRegistrationTargetImage) << endl; 
  */

  cout << "_reconstructed: " 
       << SumImage(_reconstructed) << endl;

  cout << "_mask: "
       << SumImage(_mask) << endl;
}

inline void irtkReconstruction::PrintVectorSums(vector<irtkRealImage> images, 
    string name) {
  for (int i = 0; i < (int) images.size(); i++) {
    cout << fixed << name << "[" << i << "]: " << SumImage(images[i]) << endl;
  }
}

inline void irtkReconstruction::PrintAttributeVectorSums() {
  PrintVectorSums(_slices, "slices");
  PrintVectorSums(_simulatedSlices, "simulatedSlices");
  PrintVectorSums(_simulatedInside, "simulatedInside");
  PrintVectorSums(_simulatedWeights, "simulatedWeights");
  PrintVectorSums(_weights, "weights");
  PrintVectorSums(_bias, "bias");
}