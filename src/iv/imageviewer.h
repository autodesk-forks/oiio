/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2008 Larry Gritz
// 
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
// 
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 
// (this is the MIT license)
/////////////////////////////////////////////////////////////////////////////



#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <vector>

#include <QtGui>
#include <QGLWidget>

#include "imageio.h"
using namespace OpenImageIO;

class IvMainWindow;
class IvInfoWindow;
class IvPixelviewWindow;
class IvCanvas;
class IvGL;
class IvGLPixelview;



class IvImage {
public:
    IvImage (const std::string &filename);
    ~IvImage ();

    /// Read the file from disk.  Generally will skip the read if we've
    /// already got a current version of the image in memory, unless
    /// force==true.
    bool read (int subimage=0, bool force=false,
               OpenImageIO::ProgressCallback progress_callback=NULL,
               void *progress_callback_data=NULL);

    /// Initialize this IvImage with the named image file, and read its
    /// header to fill out the spec correctly.  Return true if this
    /// succeeded, false if the file could not be read.
    bool init_spec (const std::string &filename);

    /// Return info on the last error that occurred since error_message()
    /// was called.  This also clears the error message for next time.
    std::string error_message (void) {
        std::string e = m_err;
        m_err.clear();
        return e;
    }

    /// Return a reference to the image spec;
    ///
    const ImageIOFormatSpec & spec () const { return m_spec; }

    /// Return a pointer to the start of scanline #y.
    ///
    void *scanline (int y) {
        return (void *) (m_pixels + y * m_spec.scanline_bytes());
    }

    const std::string & name (void) const { return m_name; }

    float gamma (void) const { return m_gamma; }
    void gamma (float e) { m_gamma = e; }
    float exposure (void) const { return m_exposure; }
    void exposure (float e) { m_exposure = e; }

    std::string shortinfo () const;
    std::string longinfo () const;

    /// Return the index of the subimage are we currently viewing
    ///
    int subimage () const { return m_current_subimage; }

    /// Return the number of subimages in the file.
    ///
    int nsubimages () const { return m_nsubimages; }

    int nchannels () const { return m_spec.nchannels; }

    const void *pixeladdr (int x, int y) const {
        size_t p = y * m_spec.scanline_bytes() + x * m_spec.pixel_bytes();
        return &m_pixels[p];
    }
    void getpixel (int x, int y, int *pixel) const;
    void getpixel (int x, int y, float *pixel) const;

private:
    std::string m_name;        ///< Filename of the image
    int m_nsubimages;          ///< How many subimages are there?
    int m_current_subimage;    ///< Current subimage we're viewing
    ImageIOFormatSpec m_spec;  ///< Describes the image (size, etc)
    char *m_pixels;            ///< Pixel data
    char *m_thumbnail;         ///< Thumbnail image
    bool m_spec_valid;         ///< Is the spec valid
    bool m_pixels_valid;       ///< Image is valid
    bool m_thumbnail_valid;    ///< Thumbnail is valid
    bool m_badfile;            ///< File not found
    std::string m_err;         ///< Last error message
    float m_gamma;             ///< Gamma correction of this image
    float m_exposure;          ///< Exposure gain of this image, in stops
    mutable std::string m_shortinfo;
    mutable std::string m_longinfo;

    // An IvImage can be in one of several states:
    //   * Uninitialized
    //         (m_name.empty())
    //   * Broken -- couldn't ever open the file
    //         (m_badfile == true)
    //   * Non-resident, ignorant -- know the name, nothing else
    //         (! m_name.empty() && ! m_badfile && ! m_spec_valid)
    //   * Non-resident, know spec, but the spec is valid
    //         (m_spec_valid && ! m_pixels)
    //   * Pixels loaded from disk, currently accurate
    //         (m_pixels && m_pixels_valid)

};



class ImageViewer : public QMainWindow
{
    Q_OBJECT

public:
    ImageViewer();
    ~ImageViewer();

    enum ChannelView {
        channelRed=0, channelGreen=1, channelBlue=2, channelAlpha=3,
        channelFullColor = -1, channelLuminance = -2
    };

    /// Tell the viewer about an image, but don't load it yet.  If
    /// getspec is true, open the file just enough to get the
    /// specification.
    void add_image (const std::string &filename, bool getspec=true);

    /// View this image.
    ///
    void current_image (int newimage);

    /// Which image index are we viewing?
    ///
    int current_image (void) const { return m_current_image; }

    /// View a particular channel
    ///
    void viewChannel (ChannelView c);

    /// Which channel are we viewing?
    ///
    int current_channel (void) const { return m_current_channel; }

    /// Return the current zoom level.  1.0 == 1:1 pixel ratio.  Positive
    /// is a "zoom in" (closer/maxify), negative is zoom out (farther/minify).
    float zoom (void) const { return m_zoom; }

    /// Set a new zoom level.
    ///
    void zoom (float newzoom);

    /// Return a ptr to the current image, or NULL if there is no
    /// current image.
    IvImage *cur (void) const {
        if (m_images.empty())
            return NULL;
        return m_current_image >= 0 ? m_images[m_current_image] : NULL;
    }

    /// Return a ref to the current image spec, or NULL if there is no
    /// current image.
    const ImageIOFormatSpec *curspec (void) const {
        IvImage *img = cur();
        return img ? &img->spec() : NULL;
    }

private slots:
    void open();                        ///< Dialog to open new image from file
    void reload();                      ///< Reread current image from disk
    void closeImg();                    ///< Close the current image
    void print();                       ///< Print current image
    void zoomIn();                      ///< Zoom in to next power of 2
    void zoomOut();                     ///< Zoom out to next power of 2
    void normalSize();                  ///< Adjust zoom to 1:1
    void fitImageToWindow();            ///< Adjust zoom to fit window exactly
    void fitWindowToImage();            ///< Resize window to fit image exactly
    void fullScreenToggle();            ///< Toggle full screen mode
    void about();                       ///< Show "about iv" dialog
    void prevImage();                   ///< View previous image in sequence
    void nextImage();                   ///< View next image in sequence
    void toggleImage();                 ///< View most recently viewed image
    void exposureMinusOneTenthStop();   ///< Decrease exposure 1/10 stop
    void exposureMinusOneHalfStop();    ///< Decrease exposure 1/2 stop
    void exposurePlusOneTenthStop();    ///< Increase exposure 1/10 stop
    void exposurePlusOneHalfStop();     ///< Increase exposure 1/2 stop
    void gammaMinus();                  ///< Decrease gamma 0.05
    void gammaPlus();                   ///< Increase gamma 0.05
    void viewChannelFull();             ///< View RGB
    void viewChannelRed();              ///< View just red as gray
    void viewChannelGreen();            ///< View just green as gray
    void viewChannelBlue();             ///< View just blue as gray
    void viewChannelAlpha();            ///< View alpha as gray
    void viewChannelLuminance();        ///< View luminance as gray
    void viewChannelPrev();             ///< View just prev channel as gray
    void viewChannelNext();             ///< View just next channel as gray
    void viewSubimagePrev();            ///< View prev subimage
    void viewSubimageNext();            ///< View next subimage
    void showInfoWindow();              ///< View extended info on image
    void showPixelviewWindow();         ///< View closeup pixel view
private:
    void createActions ();
    void createMenus ();
    void createToolBars ();
    void createStatusBar ();
    void readSettings ();
    void writeSettings ();
    void updateActions ();
    void displayCurrentImage ();
    void updateTitle ();
    void updateStatusBar ();
    void keyPressEvent (QKeyEvent *event);
    void resizeEvent (QResizeEvent *event);

    IvGL *glwin;
    IvInfoWindow *infoWindow;
    IvPixelviewWindow *pixelviewWindow;

#ifndef QT_NO_PRINTER
    QPrinter printer;
#endif

    QAction *openAct, *reloadAct, *closeImgAct;
    QAction *printAct;
    QAction *exitAct;
    QAction *gammaPlusAct, *gammaMinusAct;
    QAction *exposurePlusOneTenthStopAct, *exposurePlusOneHalfStopAct;
    QAction *exposureMinusOneTenthStopAct, *exposureMinusOneHalfStopAct;
    QAction *viewChannelFullAct, *viewChannelRedAct, *viewChannelGreenAct;
    QAction *viewChannelBlueAct, *viewChannelAlphaAct, *viewChannelLuminanceAct;
    QAction *viewChannelPrevAct, *viewChannelNextAct;
    QAction *viewSubimagePrevAct, *viewSubimageNextAct;
    QAction *zoomInAct;
    QAction *zoomOutAct;
    QAction *normalSizeAct;
    QAction *fitWindowToImageAct, *fitImageToWindowAct;
    QAction *fullScreenAct;
    QAction *aboutAct;
    QAction *nextImageAct, *prevImageAct, *toggleImageAct;
    QAction *showInfoWindowAct;
    QAction *showPixelviewWindowAct;
    QMenu *fileMenu, *editMenu, /**imageMenu,*/ *viewMenu, *toolsMenu, *helpMenu;
    QMenu *expgamMenu, *channelMenu;
    QLabel *statusImgInfo, *statusViewInfo;
    QProgressBar *statusProgress;

    std::vector<IvImage *> m_images;  ///< List of images
    int m_current_image;              ///< Index of current image, -1 if none
    int m_current_channel;            ///< Channel we're viewing: ChannelViews
    int m_last_image;                 ///< Last image we viewed
    float m_zoom;                     ///< Zoom amount (positive maxifies)
    bool m_fullscreen;                ///< Full screen mode

    // What zoom do we need to fit these window dimensions?
    float zoom_needed_to_fit (int w, int h);

    friend class IvCanvas;
    friend class IvGL;
    friend class IvGLPixelview;
    friend class IvPixelviewWindow;
    friend bool image_progress_callback (void *opaque, float done);
};



class IvInfoWindow : public QDialog
{
    Q_OBJECT
public:
    IvInfoWindow (ImageViewer &viewer, bool visible=true);
    void update (IvImage *img);
    
private:
    QPushButton *closeButton;
    QScrollArea *scrollArea;
    QLabel *infoLabel;

    ImageViewer &m_viewer;
    bool m_visible;
};



class IvPixelviewWindow : public QDialog
{
    Q_OBJECT
public:
    IvPixelviewWindow (ImageViewer &viewer, bool visible=true);
    void update (IvImage *img);

private:
    QPushButton *closeButton;
    IvGLPixelview *closeup;
    QLabel *infoLabel;

    ImageViewer &m_viewer;
    bool m_visible;
};



class IvGL : public QGLWidget
{
Q_OBJECT
public:
    IvGL (QWidget *parent, ImageViewer &viewer);
    ~IvGL ();

    /// Update the image texture.
    ///
    void update (IvImage *img);

    /// Update the zoom
    ///
    void zoom (float newzoom);

    void trigger_redraw (void) { glDraw(); }

    void pan (float dx, float dy);

    /// Which pixel is the mouse over?
    ///
    void get_focus_pixel (int &x, int &y);

protected:
    void initializeGL ();
    void resizeGL (int w, int h);
    void paintGL ();
private:
    ImageViewer &m_viewer;            ///< Backpointer to viewer
    bool m_pixelview;                 ///< Is this a closeup pixelview window?
    bool m_shaders_created;           ///< Have the shaders been created?
    GLuint m_vertex_shader;           ///< Vertex shader id
    GLuint m_fragment_shader;         ///< Fragment shader id
    GLuint m_shader_program;          ///< GL shader program id
    bool m_tex_created;               ///< Have the textures been created?
    GLuint m_texid;                   ///< Texture holding current imag
    bool m_dragging;                  ///< Are we dragging?
    int m_mousex, m_mousey;           ///< Last mouse position
    Qt::MouseButton m_drag_button;    ///< Button on when dragging

    float m_centerx, m_centery; ///< Where is the view centered in the img?

    void create_shaders (void);
    void create_textures (void);
    void useshader (void);
    void clamp_view_to_window ();

    void mousePressEvent (QMouseEvent *event);
    void mouseReleaseEvent (QMouseEvent *event);
    void mouseMoveEvent (QMouseEvent *event);
    void wheelEvent (QWheelEvent *event);

    void remember_mouse (const QPoint &pos) {
        m_mousex = pos.x();
        m_mousey = pos.y();
    }

    friend class IvGLPixelview;
    typedef QGLWidget parent_t;
};



class IvGLPixelview : public QGLWidget
{
Q_OBJECT
public:
    IvGLPixelview (ImageViewer &viewer);
    ~IvGLPixelview ();

    void trigger_redraw (void) { glDraw(); }
    
protected:
    void initializeGL ();
    void resizeGL (int w, int h);
    void paintGL ();
    void useshader (void);
private:
    ImageViewer &m_viewer;            ///< Backpointer to viewer

    typedef QGLWidget parent_t;
};




// Format name/value pairs as HTML table entries.
std::string html_table_row (const char *name, const std::string &value);
std::string html_table_row (const char *name, int value);
std::string html_table_row (const char *name, float value);


#endif // IMAGEVIEWER_H
