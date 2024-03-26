// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import Qt5Compat.GraphicalEffects.private

/*!
    \qmltype ZoomBlur
    \inqmlmodule Qt5Compat.GraphicalEffects
    \since Qt5Compat.GraphicalEffects 1.0
    \inherits QtQuick2::Item
    \ingroup qtgraphicaleffects-motion-blur
    \brief Applies directional blur effect towards source items center point.

    Effect creates perceived impression that the source item appears to be
    moving towards the center point in Z-direction or that the camera appears
    to be zooming rapidly. Other available motion blur effects are
    \l{Qt5Compat.GraphicalEffects::DirectionalBlur}{DirectionalBlur}
    and \l{Qt5Compat.GraphicalEffects::RadialBlur}{RadialBlur}.

    \table
    \header
        \li Source
        \li Effect applied
    \row
        \li \image Original_bug.png
        \li \image ZoomBlur_bug.png
    \endtable

    \note This effect is available when running with OpenGL.

    \section1 Example

    The following example shows how to apply the effect.
    \snippet ZoomBlur-example.qml example

*/
Item {
    id: rootItem

    /*!
        This property defines the source item that is going to be blurred.

        \note It is not supported to let the effect include itself, for
        instance by setting source to the effect's parent.
    */
    property variant source

    /*!
        This property defines the maximum perceived amount of movement for each
        pixel. The amount is smaller near the center and reaches the specified
        value at the edges.

        The quality of the blur depends on \l{ZoomBlur::samples}{samples}
        property. If length value is large, more samples are needed to keep the
        visual quality at high level.

        The value ranges from 0.0 to inf. By default the property is set to \c
        0.0 (no blur).

        \table
        \header
        \li Output examples with different length values
        \li
        \li
        \row
            \li \image ZoomBlur_length1.png
            \li \image ZoomBlur_length2.png
            \li \image ZoomBlur_length3.png
        \row
            \li \b { length: 0.0 }
            \li \b { length: 32.0 }
            \li \b { length: 48.0 }
        \row
            \li \l samples: 24
            \li \l samples: 24
            \li \l samples: 24
        \row
            \li \l horizontalOffset: 0
            \li \l horizontalOffset: 0
            \li \l horizontalOffset: 0
        \row
            \li \l verticalOffset: 0
            \li \l verticalOffset: 0
            \li \l verticalOffset: 0
        \endtable

    */
    property real length: 0.0

    /*!
        This property defines how many samples are taken per pixel when blur
        calculation is done. Larger value produces better quality, but is slower
        to render.

        This property is not intended to be animated. Changing this property may
        cause the underlying OpenGL shaders to be recompiled.

        Allowed values are between 0 and inf (practical maximum depends on GPU).
        By default the property is set to \c 0 (no samples).

    */
    property int samples: 0

    /*!
        \qmlproperty real QtGraphicalEffects::ZoomBlur::horizontalOffset
        \qmlproperty real QtGraphicalEffects::ZoomBlur::verticalOffset

        These properties define an offset in pixels for the blur direction
        center point.

        The values range from -inf to inf. By default these properties are set
        to \c 0.

        \table
        \header
        \li Output examples with different horizontalOffset values
        \li
        \li
        \row
            \li \image ZoomBlur_horizontalOffset1.png
            \li \image ZoomBlur_horizontalOffset2.png
            \li \image ZoomBlur_horizontalOffset3.png
        \row
            \li \b { horizontalOffset: 100.0 }
            \li \b { horizontalOffset: 0.0 }
            \li \b { horizontalOffset: -100.0 }
        \row
            \li \l samples: 24
            \li \l samples: 24
            \li \l samples: 24
        \row
            \li \l length: 32
            \li \l length: 32
            \li \l length: 32
        \row
            \li \l verticalOffset: 0
            \li \l verticalOffset: 0
            \li \l verticalOffset: 0
        \endtable
    */
    property real horizontalOffset: 0.0
    property real verticalOffset: 0.0

    /*!
        This property defines the blur behavior near the edges of the item,
        where the pixel blurring is affected by the pixels outside the source
        edges.

        If the property is set to \c true, the pixels outside the source are
        interpreted to be transparent, which is similar to OpenGL
        clamp-to-border extension. The blur is expanded slightly outside the
        effect item area.

        If the property is set to \c false, the pixels outside the source are
        interpreted to contain the same color as the pixels at the edge of the
        item, which is similar to OpenGL clamp-to-edge behavior. The blur does
        not expand outside the effect item area.

        By default, the property is set to \c false.

    */
    property bool transparentBorder: false

    /*!
        This property allows the effect output pixels to be cached in order to
        improve the rendering performance.

        Every time the source or effect properties are changed, the pixels in
        the cache must be updated. Memory consumption is increased, because an
        extra buffer of memory is required for storing the effect output.

        It is recommended to disable the cache when the source or the effect
        properties are animated.

        By default, the property is set to \c false.
    */
    property bool cached: false

    SourceProxy {
        id: sourceProxy
        input: rootItem.source
        sourceRect: rootItem.transparentBorder ? Qt.rect(-1, -1, parent.width + 2.0, parent.height + 2.0) : Qt.rect(0, 0, 0, 0)
    }

    ShaderEffectSource {
        id: cacheItem
        anchors.fill: shaderItem
        visible: rootItem.cached
        smooth: true
        sourceItem: shaderItem
        live: true
        hideSource: visible
    }

    ShaderEffect {
        id: shaderItem
        property variant source: sourceProxy.output
        property variant center: Qt.point(0.5 + rootItem.horizontalOffset / width, 0.5 + rootItem.verticalOffset / height)
        property real len: rootItem.length
        property bool transparentBorder: rootItem.transparentBorder
        property real samples: rootItem.samples
        property real weight: 1.0 / Math.max(1.0, rootItem.samples)
        property variant expandPixels: transparentBorder ? Qt.size(rootItem.samples, rootItem.samples) : Qt.size(0,0)
        property variant expand: transparentBorder ? Qt.size(expandPixels.width / width, expandPixels.height / height) : Qt.size(0,0)
        property variant delta: Qt.size(1.0 / rootItem.width, 1.0 / rootItem.height)

        x: transparentBorder ? -expandPixels.width - 1 : 0
        y: transparentBorder ? -expandPixels.height - 1 : 0
        width: transparentBorder ? parent.width + 2.0 * expandPixels.width + 2 : parent.width
        height: transparentBorder ? parent.height + 2.0 * expandPixels.height + 2 : parent.height

        property string fragmentShaderSkeleton: "#version 440
            layout(location = 0) in vec2 qt_TexCoord0;
            layout(location = 0) out vec4 fragColor;

            layout(std140, binding = 0) uniform buf {
                mat4 qt_Matrix;
                float qt_Opacity;
                float len;
                float weight;
                float samples;
                vec2 center;
                vec2 expand;
                vec2 delta;
            };
            layout(binding = 1) uniform sampler2D source;

            void main() {
                vec2 texCoord = qt_TexCoord0;
                vec2 centerCoord = center;

                PLACEHOLDER_EXPAND_STEPS

                vec2 dir = vec2(centerCoord.x - texCoord.s, centerCoord.y - texCoord.t);
                dir /= max(1.0, length(dir) * 2.0);
                vec2 shift = delta * len * dir * 2.0 / max(1.0, samples - 1.0);
                fragColor = vec4(0.0);

                PLACEHOLDER_UNROLLED_LOOP

                fragColor *= weight * qt_Opacity;
            }
        "

        function buildFragmentShader() {
            var shader = fragmentShaderSkeleton
            var expandSteps = ""

            if (transparentBorder) {
                expandSteps += "centerCoord = (centerCoord - expand) / (1.0 - 2.0 * expand);"
                expandSteps += "texCoord = (texCoord - expand) / (1.0 - 2.0 * expand);"
             }

             var unrolledLoop = "fragColor += texture(source, texCoord);\n"

             if (rootItem.samples > 1) {
                 unrolledLoop = ""
                 for (var i = 0; i < rootItem.samples; i++)
                      unrolledLoop += "fragColor += texture(source, texCoord); texCoord += shift;\n"
             }

             shader = shader.replace("PLACEHOLDER_EXPAND_STEPS", expandSteps)
             fragmentShader = ShaderBuilder.buildFragmentShader(shader.replace("PLACEHOLDER_UNROLLED_LOOP", unrolledLoop))
        }

        onFragmentShaderChanged: sourceChanged()
        onSamplesChanged: buildFragmentShader()
        onTransparentBorderChanged: buildFragmentShader()
        Component.onCompleted: buildFragmentShader()
    }
}
