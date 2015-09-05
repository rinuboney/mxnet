/*!
 * \file image_augmenter_opencv.hpp
 * \brief threaded version of page iterator
 * \author Naiyan Wang, Tianqi Chen, Tianjun Xiao
 */
#ifndef MXNET_IO_IMAGE_AUGMENTER_H_
#define MXNET_IO_IMAGE_AUGMENTER_H_

#include <opencv2/opencv.hpp>
#include "../common/utils.h"

namespace mxnet {
namespace io {
/*! \brief image augmentation parameters*/
struct ImageAugmentParam : public dmlc::Parameter<ImageAugmentParam> {
  /*! \brief whether we do random cropping */
  bool rand_crop;
  /*! \brief whether we do nonrandom croping */
  int crop_y_start;
  /*! \brief whether we do nonrandom croping */
  int crop_x_start;
  /*! \brief [-max_rotate_angle, max_rotate_angle] */
  int max_rotate_angle;
  /*! \brief max aspect ratio */
  float max_aspect_ratio;
  /*! \brief random shear the image [-max_shear_ratio, max_shear_ratio] */
  float max_shear_ratio;
  /*! \brief max crop size */
  int max_crop_size;
  /*! \brief min crop size */
  int min_crop_size;
  /*! \brief max scale ratio */
  float max_random_scale;
  /*! \brief min scale_ratio */
  float min_random_scale;
  /*! \brief min image size */
  float min_img_size;
  /*! \brief max image size */
  float max_img_size;
  /*! \brief rotate angle */
  int rotate;
  /*! \brief filled color while padding */
  int fill_value;
  /*! \brief whether to mirror the image */
  bool mirror;
  /*! \brief whether to perform rand mirror the image */
  bool rand_mirror;
  /*! \brief mean file string*/
  std::string mean_img;
  /*! \brief mean value for r channel */
  float mean_r;
  /*! \brief mean value for g channel */
  float mean_g;
  /*! \brief mean value for b channel */
  float mean_b;
  /*! \brief shape of the image data*/
  TShape input_shape;
  /*! \brief maximum ratio of contrast variation */
  float max_random_contrast_;
  /*! \brief maximum value of illumination variation */
  float max_random_illumination_;
  // declare parameters
  // TODO: didn't understand the range for some params
  DMLC_DECLARE_PARAMETER(ImageAugmentParam) {
    DMLC_DECLARE_FIELD(rand_crop_).set_default(true)
        .describe("Whether we de random cropping");
    DMLC_DECLARE_FIELD(crop_y_start).set_default(-1)
        .describe("Where to nonrandom crop on y");
    DMLC_DECLARE_FIELD(crop_x_start).set_default(-1)
        .describe("Where to nonrandom crop on x");
    DMLC_DECLARE_FIELD(max_rotate_angle).set_default(0.0f)
        .describe("Rotate can be [-max_rotate_angle, max_rotate_angle]");
    DMLC_DECLARE_FIELD(max_aspect_ratio).set_default(0.0f)
        .describe("Max aspect ratio");
    DMLC_DECLARE_FIELD(max_shear_ratio).set_default(0.0f)
        .describe("Shear rotate can be made between [-max_shear_ratio_, max_shear_ratio_]");
    DMLC_DECLARE_FIELD(max_crop_size).set_default(-1)
        .describe("Maximum crop size");
    DMLC_DECLARE_FIELD(min_crop_size).set_default(-1)
        .describe("Minimum crop size");
    DMLC_DECLARE_FIELD(max_random_scale).set_default(1.0f)
        .describe("Maxmum scale ratio");
    DMLC_DECLARE_FIELD(min_random_scale).set_default(1.0f)
        .describe("Minimum scale ratio");       
    DMLC_DECLARE_FIELD(max_img_size).set_default(1e10f)
        .describe("Maxmum image size");
    DMLC_DECLARE_FIELD(min_img_size).set_default(0.0f)
        .describe("Minimum image size");
    DMLC_DECLARE_FIELD(rotate).set_default(-1.0f)
        .describe("Rotate angle");
    DMLC_DECLARE_FIELD(fill_value).set_default(255)
        .describe("Filled value while padding");
    DMLC_DECLARE_FIELD(mirror).set_default(false)
        .describe("Whether to mirror the image");
    DMLC_DECLARE_FIELD(rand_mirror).set_default(false)
        .describe("Whether to mirror the image randomly");
    DMLC_DECLARE_FIELD(mean_img).set_default("")
        .describe("Mean Image to be subtracted");
    DMLC_DECLARE_FIELD(mean_r).set_default(0.0f)
        .describe("Mean value on R channel");
    DMLC_DECLARE_FIELD(mean_g).set_default(0.0f)
        .describe("Mean value on G channel");   
    DMLC_DECLARE_FIELD(mean_b).set_default(0.0f)
        .describe("Mean value on B channel");
    float input_shape_default = {3, 224, 224};
    DMLC_DECLARE_FIELD(input_shape).set_default(TShape(input_shape_default, input_shape_default + 3))
        .set_expect_ndim(3).enforce_nonzero()
        .describe("Input shape of the neural net");
    DMLC_DECLARE_FIELD(max_random_contrast).set_default(0.0f)
        .describe("Maximum ratio of contrast variation");
    DMLC_DECLARE_FIELD(max_random_illumination).set_default(0.0f)
        .describe("Maximum value of illumination variation");
  }
};

/*! \brief helper class to do image augmentation */
class ImageAugmenter {
 public:
  // contructor
  ImageAugmenter(void)
      : tmpres_(false),
        rotateM_(2, 3, CV_32F) {
  }
  virtual ~ImageAugmenter() {
  }
  // TODO: Hack the shape and rotate list, didn't use param
  virtual void Init(const std::vector<std::pair<std::string, std::string> >& kwargs) {
    std::vector<std::pair<std::string, std::string> > kwargs_left;
    kwargs_left = param_.InitAllowUnknown(kwargs);
    for (size_t i = 0; i < kwargs_left.size(); i++) {
        if (!strcmp(kwargs_left[i].first.c_str(), "rotate_list")) {
          const char* val = kwargs_left[i].second.c_str();
          const char *end = val + strlen(val);
          char buf[128];
          while (val < end) {
            sscanf(val, "%[^,]", buf);
            val += strlen(buf) + 1;
            rotate_list_.push_back(atoi(buf));
          }
        }
    }
    if (param_.mean_img.length() != 0) {
      dmlc::Stream *fi = dmlc::Stream::Create(param_.mean_img.c_str(), "r", true);
      if (fi == NULL) {
        this->CreateMeanImg();
      } else {
        if (param_.silent == 0) {
          printf("loading mean image from %s\n", param_.mean_img.c_str());
        }
        meanimg_.LoadBinary(*fi);
        delete fi;
        meanfile_ready_ = true;
      }
    }
  }
  /*!
   * \brief augment src image, store result into dst
   *   this function is not thread safe, and will only be called by one thread
   *   however, it will tries to re-use memory space as much as possible
   * \param src the source image
   * \param source of random number
   * \param dst the pointer to the place where we want to store the result
   */
  virtual cv::Mat OpencvProcess(const cv::Mat &src,
                          common::RANDOM_ENGINE *prnd) {
    // shear
    float s = NextDouble(prnd) * param_.max_shear_ratio * 2 - param_.max_shear_ratio;
    // rotate
    int angle = NextUInt32(param_.max_rotate_angle * 2, prnd) - param_.max_rotate_angle;
    if (param_.rotate > 0) angle = param_.rotate;
    if (rotate_list_.size() > 0) {
      angle = rotate_list_[NextUInt32(rotate_list_.size() - 1, prnd)];
    }
    float a = cos(angle / 180.0 * M_PI);
    float b = sin(angle / 180.0 * M_PI);
    // scale
    float scale = NextDouble(prnd) * (param_.max_random_scale - param_.min_random_scale) + param_.min_random_scale;
    // aspect ratio
    float ratio = NextDouble(prnd) * param_.max_aspect_ratio * 2 - param_.max_aspect_ratio + 1;
    float hs = 2 * scale / (1 + ratio);
    float ws = ratio * hs;
    // new width and height
    float new_width = std::max(param_.min_img_size, std::min(param_.max_img_size, scale * src.cols));
    float new_height = std::max(param_.min_img_size, std::min(param_.max_img_size, scale * src.rows));
    //printf("%f %f %f %f %f %f %f %f %f\n", s, a, b, scale, ratio, hs, ws, new_width, new_height);
    cv::Mat M(2, 3, CV_32F);
    M.at<float>(0, 0) = hs * a - s * b * ws;
    M.at<float>(1, 0) = -b * ws;
    M.at<float>(0, 1) = hs * b + s * a * ws;
    M.at<float>(1, 1) = a * ws;
    float ori_center_width = M.at<float>(0, 0) * src.cols + M.at<float>(0, 1) * src.rows;
    float ori_center_height = M.at<float>(1, 0) * src.cols + M.at<float>(1, 1) * src.rows;
    M.at<float>(0, 2) = (new_width - ori_center_width) / 2;
    M.at<float>(1, 2) = (new_height - ori_center_height) / 2;
    cv::warpAffine(src, temp, M, cv::Size(new_width, new_height),
                     cv::INTER_LINEAR,
                     cv::BORDER_CONSTANT,
                     cv::Scalar(param_.fill_value, param_.fill_value, param_.fill_value));
    cv::Mat res = temp;
    // crop
    if (param_.max_crop_size != -1 || param_.min_crop_size != -1){
      CHECK(res.cols >= param_.max_crop_size && res.rows >= param_.max_crop_size && param_.max_crop_size >= param_.min_crop_size)
          << "input image size smaller than max_crop_size";
      mshadow::index_t rand_crop_size = NextUInt32(param_.max_crop_size- param_.min_crop_size+1, prnd)+ param_.min_crop_size;
      mshadow::index_t y = res.rows - rand_crop_size;
      mshadow::index_t x = res.cols - rand_crop_size;
      if (param_.rand_crop != 0) {
        y = NextUInt32(y + 1, prnd);
        x = NextUInt32(x + 1, prnd);
      }
      else {
        y /= 2; x /= 2;
      }
      cv::Rect roi(x, y, rand_crop_size, rand_crop_size);
      cv::resize(res(roi), res, cv::Size(param_.input_shape[1], param_.input_shape[2]));
    }
    else{
      CHECK(static_cast<mshadow::index_t>(res.cols) >= param_.input_shape[1] && static_cast<mshadow::index_t>(res.rows) >= param_.input_shape[2]) 
          << "input image size smaller than input shape";
      mshadow::index_t y = res.rows - param_.input_shape[2];
      mshadow::index_t x = res.cols - param_.input_shape[1];
      if (param_.rand_crop_ != 0) {
        y = NextUInt32(y + 1, prnd);
        x = NextUInt32(x + 1, prnd);
      }
      else {
        y /= 2; x /= 2;
      }
      cv::Rect roi(x, y, param_.input_shape[1], param_.input_shape[2]);
      res = res(roi);
    }
    return res;
  }
  /*!
   * \brief augment src image, store result into dst
   *   this function is not thread safe, and will only be called by one thread
   *   however, it will tries to re-use memory space as much as possible
   * \param src the source image
   * \param source of random number
   * \param dst the pointer to the place where we want to store the result
   */
  virtual mshadow::Tensor<cpu, 3> OpencvProcess(mshadow::Tensor<cpu, 3> data,
                                          common::RANDOM_ENGINE *prnd) {
    if (!NeedOpencvProcess()) return data;
    cv::Mat res(data.size(1), data.size(2), CV_8UC3);
    for (index_t i = 0; i < data.size(1); ++i) {
      for (index_t j = 0; j < data.size(2); ++j) {
        res.at<cv::Vec3b>(i, j)[0] = data[2][i][j];
        res.at<cv::Vec3b>(i, j)[1] = data[1][i][j];
        res.at<cv::Vec3b>(i, j)[2] = data[0][i][j];
      }
    }
    res = this->OpencvProcess(res, prnd);
    tmpres.Resize(mshadow::Shape3(3, res.rows, res.cols));
    for (index_t i = 0; i < tmpres.size(1); ++i) {
      for (index_t j = 0; j < tmpres.size(2); ++j) {
        cv::Vec3b bgr = res.at<cv::Vec3b>(i, j);
        tmpres[0][i][j] = bgr[2];
        tmpres[1][i][j] = bgr[1];
        tmpres[2][i][j] = bgr[0];
      }
    }
    return tmpres;
  }

  virtual void OpencvProcess(unsigned char *dptr, size_t sz,
                       mshadow::TensorContainer<cpu, 3> *p_data,
                       common::RANDOM_ENGINE *prnd) {
    cv::Mat buf(1, sz, CV_8U, dptr);
    cv::Mat res = cv::imdecode(buf, 1);
    res = this->OpencvProcess(res, prnd);
    p_data->Resize(mshadow::Shape3(3, res.rows, res.cols));
    for (index_t i = 0; i < p_data->size(1); ++i) {
      for (index_t j = 0; j < p_data->size(2); ++j) {
        cv::Vec3b bgr = res.at<cv::Vec3b>(i, j);
        (*p_data)[0][i][j] = bgr[2];
        (*p_data)[1][i][j] = bgr[1];
        (*p_data)[2][i][j] = bgr[0];
      }
    }
    res.release();
  }

  void TensorProcess(mshadow::TensorContainer<cpu, 3> *p_data,
                       common::RANDOM_ENGINE *prnd) {
    img_.Resize(mshadow::Shape3((*p_data).shape_[0], param_.input_shape[1], param_.input_shape[2]));
    if (param_.input_shape[1] == 1) {
      img_ = (*p_data) * param_.scale;
    } else {
      CHECK(p_data->size(1) >= param_.input_shape[1] && p_data->size(2) >= param_.input_shape[2])
          << "Data size must be bigger than the input size to net.";
      mshadow::index_t yy = p_data->size(1) - param_.input_shape[1];
      mshadow::index_t xx = p_data->size(2) - param_.input_shape[2];
      if (param_.rand_crop != 0 && (yy != 0 || xx != 0)) {
        yy = NextUInt32(yy + 1, prnd);
        xx = NextUInt32(xx + 1, prnd);
      } else {
        yy /= 2; xx /= 2;
      }
      if (p_data->size(1) != param_.input_shape[1] && param_.crop_y_start != -1) {
        yy = param_.crop_y_start;
      }
      if (p_data->size(2) != param_.input_shape[2] && param_.crop_x_start != -1) {
        xx = param_.crop_x_start;
      }
      float contrast = NextDouble(prnd) * param_.max_random_contrast * 2 - param_.max_random_contrast + 1;
      float illumination = NextDouble(prnd) * param_.max_random_illumination * 2 - param_.max_random_illumination;
      if (param_.mean_r > 0.0f || param_.mean_g > 0.0f || param_.mean_b > 0.0f) {
        // substract mean value
        (*p_data)[0] -= param_.mean_b; (*p_data)[1] -= param_.mean_g; (*p_data)[2] -= param_.mean_r;
        if ((param_.rand_mirror != 0 && NextDouble(rnd) < 0.5f) || param_.mirror == 1) {
          img_ = mirror(crop((*p_data) * contrast + illumination, img_[0].shape_, yy, xx)) * param_.scale;
        } else {
          img_ = crop((*p_data) * contrast + illumination, img_[0].shape_, yy, xx) * param_.scale ;
        }
      } else if (!meanfile_ready_ || param_.mean_img.length() == 0) {
        // do not substract anything
        if (param_.rand_mirror != 0 && NextDouble(prnd) < 0.5f) {
          img_ = mirror(crop((*p_data), img_[0].shape_, yy, xx)) * param_.scale;
        } else {
          img_ = crop((*p_data), img_[0].shape_, yy, xx) * param_.scale ;
        }
      } else {
        // substract mean image
        if ((param_.rand_mirror != 0 && NextDouble(prnd) < 0.5f) || param_.mirror == 1) {
          if (p_data->shape_ == meanimg_.shape_) {
            img_ = mirror(crop(((*p_data) - meanimg_) * contrast + illumination, img_[0].shape_, yy, xx)) * param_.scale;
          } else {
            img_ = (mirror(crop((*p_data), img_[0].shape_, yy, xx) - meanimg_) * contrast + illumination) * param_.scale;
          }
        } else {
          if (p_data->shape_ == meanimg_.shape_){
            img_ = crop(((*p_data) - meanimg_) * contrast + illumination, img_[0].shape_, yy, xx) * param_.scale;
          } else {
            img_ = ((crop((*p_data), img_[0].shape_, yy, xx) - meanimg_) * contrast + illumination) * param_.scale;
          }
        }
      }
    }
    out_.data = img_;
  } 

  inline void CreateMeanImg(void) {
    if (silent_ == 0) {
      printf("cannot find %s: create mean image, this will take some time...\n", name_meanimg_.c_str());
    }
    time_t start = time(NULL);
    unsigned long elapsed = 0;
    size_t imcnt = 1;

    CHECK(this->Next_()) << "input iterator failed.";
    meanimg_.Resize(mshadow::Shape3(shape_[0], shape_[1], shape_[2]));
    mshadow::Copy(meanimg_, img_);
    while (this->Next()) {
      meanimg_ += img_; imcnt += 1;
      elapsed = (long)(time(NULL) - start);
      if (imcnt % 1000 == 0 && silent_ == 0) {
        printf("\r                                                               \r");
        printf("[%8lu] images processed, %ld sec elapsed", imcnt, elapsed);
        fflush(stdout);
      }
    }
    meanimg_ *= (1.0f / imcnt);

    dmlc::Stream *fo = dmlc::Stream::Create(name_meanimg_.c_str(), "w");
    meanimg_.SaveBinary(*fo);
    delete fo;
    if (silent_ == 0) {
      printf("save mean image to %s..\n", name_meanimg_.c_str());
    }
    meanfile_ready_ = true;
  }


 private:
  // whether skip opencv processing
  inline bool NeedOpencvProcess(void) const {
    if (param_.max_rotate_angle > 0 || param_.max_shear_ratio > 0.0f
        || param_.rotate > 0 || rotate_list_.size() > 0) return true;
    if (param_.min_crop_size > 0 && param_.max_crop_size > 0) return true;
    return false;
  }
  // temp input space
  mshadow::TensorContainer<cpu, 3> tmpres_;
  // mean image
  mshadow::TensorContainer<cpu, 3> meanimg_;
  /*! \brief temp space */
  mshadow::TensorContainer<cpu, 3> img_;
  // temporal space
  cv::Mat temp_;
  // rotation param
  cv::Mat rotateM_;
  // whether the mean file is ready
  bool menafile_ready_;
  // parameters
  ImageAugmentParam param_;
  /*! \brief input shape */
  mshadow::Shape<4> shape_;
  /*! \brief list of possible rotate angle */
  std::vector<int> rotate_list_;
};
}  // namespace io
}  // namespace cxxnet
#endif
