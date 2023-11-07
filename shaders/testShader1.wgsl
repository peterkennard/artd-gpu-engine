R"(  // this here so can be included in C++ as a string - reader needs to strip out

struct VertexInput {
	@builtin(instance_index) instanceIx: u32,
	@location(0) position: vec3f,
	@location(1) normal: vec3f, // new attribute
	@location(2) uv: vec2f,
};

// SEE: https://www.w3.org/TR/WGSL/#alignment-and-size
// for alignment specs

struct LightData {
    pose:  mat3x3f,
    position: vec3f,
    diffuse: vec3f,
    lightType: u32,
    vec0: vec4f,  // multi-use depending on type
};

/**
 * A structure holding the value of scene global uniforms
 */
struct SceneUniforms {
    projectionMatrix: mat4x4f,
    viewMatrix: mat4x4f,
    vpMatrix: mat4x4f,
    eyePose:  mat4x4f,  // need inverse view ? or this ?
    test: mat4x4f,
    time: f32,
    numLights: u32,
    pad1: f32,
    pad2: f32,
    lights: array<LightData,64>
};

struct InstanceData {
    modelMatrix: mat4x4f,
    materialId: u32,
    objectId: u32,
    unused2_: u32, // pad for 16 bytes alignment (needed ? )
};

struct MaterialData {
    diffuse: vec3f,
    unused_: f32,  // id ? we need transparency ??
    specular: vec3f,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) worldPos: vec4f,
    @location(1) normal: vec3f,
    @location(2) uv:  vec2f,  // TBD unused now
    @location(3) @interpolate(flat) materialIx: u32
};



// whole frame bind group
@group(0) @binding(0) var<uniform> scnUniforms: SceneUniforms;
// TODO: not sure if this is right for the array guessing
@group(0) @binding(1) var<storage> instanceArray : array<InstanceData>;
@group(0) @binding(2) var<storage> materialArray : array<MaterialData>;
@group(0) @binding(3) var sampler0: sampler;

// material bind group
@group(1) @binding(0) var texture0: texture_2d<f32>;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var out: VertexOutput;
// Use the instance index to retrieve the matrix !!  indexData[in.instanceIx]

    let mMat = instanceArray[in.instanceIx].modelMatrix;

    out.worldPos = mMat * vec4f(in.position, 1.0);
    out.position = scnUniforms.vpMatrix * out.worldPos;
	out.uv = in.uv;

	// Forward the normal
	// TODO: pass in a normal "pose" matrix? in instance data ?
    let nMat = mat3x3f(mMat[0].xyz, mMat[1].xyz, mMat[2].xyz);
    out.normal = normalize(nMat * in.normal);

	out.materialIx = instanceArray[in.instanceIx].materialId;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
	let normal = normalize(in.normal); // the interpolator doesn't keep it normalized !!! ie: rotate it !

    let material = materialArray[in.materialIx]; // indirection or by value ? or is it a reference ?

	// We remap UV coords to actual texel coordinates
//	let texelCoords = vec2i(in.uv * vec2f(textureDimensions(tex0)));

	// And we fetch a texel from the texture
//	let color = textureLoad(tex0, texelCoords, 0).rgb;

//    let texDim = textureDimensions(texture0);
//    let hasTex0 = (texDim.x + texDim.y) != 2;

    var diffuseMult = vec3f(0,0,0);
    var specularMult = vec3f(0,0,0);

    for(var lix = u32(0); lix < scnUniforms.numLights; lix += 1)  {

        let light = scnUniforms.lights[lix];

        switch light.lightType {
            case 0: { // directional

                var incidence = dot(light.pose[2],normal); // z axis of rotation, light direction - pre normalized

                let wrap = light.vec0.x;
                var shading = incidence + wrap;
                shading *= 1.0/(1.+wrap);

//  https://en.wikipedia.org/wiki/Blinn%E2%80%93Phong_reflection_model
//                //Calculate the half vector between the light vector and the view vector.
//                //This is typically slower than calculating the actual reflection vector
//                // due to the normalize function's reciprocal square root
//                float3 H = normalize(lightDir + viewDir);
//
//                //Intensity of the specular light
//                float NdotH = dot(normal, H);
//                intensity = pow(saturate(NdotH), specularHardness);
//
//                //Sum up the specular light factoring
//                OUT.Specular = intensity * light.specularColor * light.specularPower / distance;
//
//      https://registry.khronos.org/OpenGL-Refpages/gl4/html/reflect.xhtml
//      reflection dir = glsl::reflect(I, N) For a given incident vector vec
//      and surface normal N reflect returns the reflection direction
//      calculated as I - 2.0 * dot(N, I) * N.

                let arg1 = scnUniforms.test[0].y;

                if(incidence > 0.) {
//                if(shading > 0.) {
                    var reflectDirection = reflect(light.pose[2], normal);
                    var toView = normalize(in.worldPos.xyz - scnUniforms.eyePose[3].xyz); // vector to eye from position.
                    // TODO: this is not really right - phong like - but acceptable for now
                    // TODO: really need to have a bespoke curve for different materials and or specularity map.
                    var d = dot(reflectDirection, toView) + (wrap * .002);
                    var specPower = 0.0;

                    if(d > 0.0 && arg1 > 0.0) { // TODO: material specularity
                        specPower = max(pow(d, 1.0 + (arg1+.04) * 1000.), 0.0) * (1.0 - (.4*incidence));
                        if(specPower > 1.0) {
                            specPower = 1.0;
                        }
                    }

                    // 1 minute × π/(60 × 180) = 0.0002909 radians
                    // sun == 32 minutes ( 16 minutes radius )
                    // so if d > cos 16 minutes then size of sun
//                    let sc = cos(arg1 * (3.14159f/2.f)); // if(d > sinCompare)  // way bigger than sun or moon which cos(.0002909)
//                    if(d > sc)
//                    {
//                        var dToSc = 0.0;
//                        if(sc > 0) {
//                            dToSc = 1.0 - (1.0 - d)/(1.0 - sc);
//                        }
//                        // lerp
//                        // return (start_value + (end_value - start_value) * pct);
//                        dToSc = (.3 + (1.0 - .3) * dToSc);
//                        if(dToSc > 0.0) {
//                            specPower = dToSc;
//                        }
//
//                        // (dToSc * 4); // (1.0 - (d - sc));
//                        // specPower = 1.0;
//                    } else {
//
//                        var dToSc = 0.0;
//                        if(sc > 0) {
//                            dToSc = (d/sc) * 50.0;
//                        }
//                        dToSc -= 49.999;
//
//                        specPower = dToSc * .3;
//
//                        // sc += .0000001;
//                        // specPower = 0.0; //1.0 - ( sc - d )/sc;
//
//                    }  // .2 * (pow(max(d, 0.0), 10.1 ) - 1060.0); // shininess);

                    if(specPower < 0.0) {
                        specPower = 0.;
                    }
                    specularMult = vec3f(specPower,specPower,specPower);
                }

//	            var incidence = dot(light.pose[2], normal);
//                if(incidence > .98) {
//                    var inverseView = scnUniforms.eyePose;
//                    var eyePos = vec3(inverseView[3].xyz);
//                    var viewNormal = normalize(eyePos - vec3f(in.position.xyz));
//                    var reflectDirection = reflect(light.pose[2], normal);
//                    var d = .2 * dot(reflectDirection, viewNormal);
//		            var specPower = pow(max(d, 0.0), .01 ); // shininess);
//		            specularMult = vec3f(specPower,specPower,specPower);
//		        }

                if(shading > 0) {
                    diffuseMult += shading;
                }
            }
            default: {
                // do nothing !
            }
        }
    }

	var color = material.diffuse * diffuseMult + specularMult; // shading;
    if(color.x > 1.0) {
        color.x = 1.0;
    }
    if(color.y > 1.0) {
        color.y = 1.0;
    }
    if(color.z > 1.0) {
        color.z = 1.0;
    }

	// Gamma-correction
	let corrected_color = color; // pow(color, vec3f(2.2));
    return vec4f(corrected_color,1.0); // corrected_color, scnUniforms.color.a);
}
// )";
