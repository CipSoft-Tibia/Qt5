// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import Qt5Compat.GraphicalEffects.private

/*!
    \qmltype DirectionalBlur
    \inqmlmodule Qt5Compat.GraphicalEffects
    \since QtGraphicalEffects 1.0
    \inherits QtQuick2::Item
    \ingroup qtgraphicaleffects-motion-blur
    \brief Applies blur effect to the specified direction.

    Effect creates perceived impression that the source item appears to be
    moving in the direction of the blur. Blur is applied to both sides of
    each pixel, therefore setting the direction to 0 and 180 provides the
    same result.

    Other available motionblur effects are \l{Qt5Compat.GraphicalEffects::ZoomBlur}{ZoomBlur} and
    \l{Qt5Compat.GraphicalEffects::RadialBlur}{RadialBlur}.

    \table
    \header
        \li Source
        \li Effect applied
    \row
        \li \image Original_bug.png
        \li \image DirectionalBlur_bug.png
    \endtable

    \note This effect is available when running with OpenGL.

    \section1 Example

    The following example shows how to apply the effect.
    \snippet DirectionalBlur-example.qml example

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
        This property defines the perceived amount of movement for each pixel.
        The movement is divided evenly to both sides of each pixel.

        The quality of the blur depends on \l{DirectionalBlur::samples}{samples}
        property. If length value is large, more samples are needed to keep the
        visual quality at high level.

        The value ranges from 0.0 to inf.
        By default the property is set to \c 0.0 (no blur).

        \table
        \header
        \li Output examples with different length values
        \li
        \li
        \row
            \li \image DirectionalBlur_length1.png
            \li \image DirectionalBlur_length2.png
            \li \image DirectionalBlur_length3.png
        \row
            \li \b { length: 0.0 }
            \li \b { length: 32.0 }
            \li \b { length: 48.0 }
        \row
            \li \l samples: 24
            \li \l samples: 24
            \li \l samples: 24
        \row
            \li \l angle: 0
            \li \l angle: 0
            \li \l angle: 0
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
        This property defines the direction for the blur. Blur is applied to
        both sides of each pixel, therefore setting the direction to 0 and 180
        produces the same result.

        The value ranges from -180.0 to 180.0.
        By default the property is set to \c 0.0.

        \table
        \header
        \li Output examples with different angle values
        \li
        \li
        \row
            \li \image DirectionalBlur_angle1.png
            \li \image DirectionalBlur_angle2.png
            \li \image DirectionalBlur_angle3.png
        \row
            \li \b { angle: 0.0 }
            \li \b { angle: 45.0 }
            \li \b { angle: 90.0 }
        \row
            \li \l samples: 24
            \li \l samples: 24
            \li \l samples: 24
        \row
            \li \l length: 32
            \li \l length: 32
            \li \l length: 32
        \endtable

    */
    property real angle: 0.0

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
        property real len: rootItem.length
        property bool transparentBorder: rootItem.transparentBorder
        property real samples: rootItem.samples
        property real weight: 1.0 / Math.max(1.0, rootItem.samples)
        property variant expandPixels: transparentBorder ? Qt.size(rootItem.samples, rootItem.samples) : Qt.size(0,0)
        property variant expand: transparentBorder ? Qt.size(expandPixels.width / width, expandPixels.height / height) : Qt.size(0,0)
        property variant delta: Qt.size(1.0 / rootItem.width * Math.cos((rootItem.angle + 90) * Math.PI/180), 1.0 / rootItem.height * Math.sin((rootItem.angle + 90) * Math.PI/180))

        x: transparentBorder ? -expandPixels.width - 1: 0
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
                float samples;
                float weight;
                vec2 expand;
                vec2 delta;
            };
            layout(binding = 1) uniform sampler2D source;

            void main(void) {
                vec2 shift = delta * len / max(1.0, samples - 1.0);
                vec2 texCoord = qt_TexCoord0;
                fragColor = vec4(0.0);

                PLACEHOLDER_EXPAND_STEPS

                texCoord -= shift * max(0.0, samples - 1.0) * 0.5;

                PLACEHOLDER_UNROLLED_LOOP

                fragColor *= weight * qt_Opacity;
           }
        "

        function buildFragmentShader() {
            var shader = fragmentShaderSkeleton
            var expandSteps = ""

            if (transparentBorder) {
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
