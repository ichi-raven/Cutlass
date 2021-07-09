
struct VSOutput
{
    float4 Position         : SV_POSITION;
    float2 UV               : TEXCOORD0;
};

cbuffer CB : register(b0, space0)
{
    float2 surfaceSize;
    float2 scale;
    float2 trans;
    float  angle;
    uint   frame;
}

Texture2D    tex      : register(t0, space1);
SamplerState Sampler : register(s0, space1);


VSOutput VSMain(uint id : SV_VERTEXID)
{
    VSOutput result = (VSOutput)0;

    float2  pos;

    pos.x = (float)(id % 2);
    pos.y = (float)(id / 2);

    pos.x *= scale.x;//画像の頂点になった
    pos.y *= scale.y;

    //スケール変換終了

    float2 center = scale / 2;

    float2 relative = pos - center;

    float2 sc = float2(cos(angle), sin(angle));

    float2 spinned = float2(
                               sc.x * relative.x - sc.y * relative.y,
                               sc.y * relative.x + sc.x * relative.y
                           );
    pos = spinned + center;

    //回転変換終了
    
    float2 transYRev = float2(trans.x, trans.y) * 2;

    pos += transYRev;
    pos -= center;

    //平行移動終了

    //座標修正
    //float2 scalePer = scale / surfaceSize;//ビューポートと画像のサイズの比

    //pos /= surfaceSize / 2;

    pos.x -= 1.f;
    pos.y -= 1.f;

    result.Position = float4(pos.xy, 0, 1);

    float2 t;
    t.x = (float)(id % 2);
    t.y = (float)(id / 2);

    result.UV = t;

    return result;
}

float4 PSMain(VSOutput In) : SV_TARGET
{
    float4 color = tex.Sample(Sampler, In.UV);
    color.w = 1.f;
    return color;
}








//struct VSOutput
//{
//    float4 Position         : SV_POSITION;
//    float2 UV               : TEXCOORD0;
//};

//cbuffer CB : register(b0)
//{
//    float2 surfaceSize;
//    float2 scale;
//    float2 trans;
//    float  angle;
//    uint   frame;
//}
//
//Texture2D    tex      : register(t0);
//
//SamplerState Sampler : register(s0);
//
//
//VSOutput VSMain(uint id : SV_VERTEXID)
//{
//    VSOutput result = (VSOutput)0;
//
//    float2  pos;
//
//    pos.x = (float)(id % 2) * 2.f - 1.f;
//    pos.y = (float)(id / 2) * 2.f - 1.f;
//
//    //pos.x = (float)(id % 2) *  scale.x;//画像の頂点になった
//    //pos.y = (float)(id / 2) * -scale.y;
//
//    ////スケール変換終了
//
//    //float2 center = scale / 2;
//
//    //float2 relative = pos - center;
//
//    //float2 sc = float2(cos(angle), sin(angle));
//
//    //float2 spinned = float2(
//    //                            sc.x * relative.x - sc.y * relative.y,
//    //                            sc.y * relative.x + sc.x * relative.y
//    //                        );
//    //pos = spinned + center;
//
//    ////回転変換終了
//    //
//    //float2 transYRev = float2(trans.x, trans.y * -1);
//
//    //pos += transYRev;
//    ////pos -= center;
//
//    ////平行移動終了
//
//    ////座標修正
//    //float2 scalePer = scale / surfaceSize;//ビューポートと画像のサイズの比
//
//    //pos /= surfaceSize / 2;
//
//    //pos.x -= 1.f;
//    //pos.y += 1.f;
//
//    //float ang = frame * 3.14 / 50;
//
//    //pos.x = pos.x * cos(ang) - pos.y * sin(ang);
//    //pos.y = pos.x * sin(ang) + pos.y * cos(ang);
//
//    result.Position = float4(pos.xy, 0, 1);
//
//    float2 t;
//    t.x = (float)(id % 2);
//    t.y = 1.f - (float)(id / 2);
//
//    result.UV = t;
//
//    return result;
//}
//
//float4 PSMain(VSOutput In) : SV_TARGET
//{
//    float resolution = 100000.f / (float)((frame % 60) + 1) / 10.f;
//    int mosaicAcc = 10;
//    float2 mosaicUV = float2((float)((int)(In.UV.x * resolution) / mosaicAcc * mosaicAcc) / resolution, (float)((int)(In.UV.y * resolution) / mosaicAcc * mosaicAcc) / resolution);
//
//    //float4 color = tex.Sample(Sampler, mosaicUV);
//    float ang = frame * 3.14f / 3.1f;
//    float2 uv = mosaicUV - float2(0.5f, 0.5f);
//    //uv.x = uv.x * cos(ang) - uv.y * sin(ang);
//    //uv.y = uv.x * sin(ang) + uv.y * cos(ang);
//    uv += float2(0.5f, 0.5f);
//
//    float4 color = tex.Sample(Sampler, uv);
//
//    //if (frame % 20 < 10)
//    //{
//        //if (color.r + color.g + color.b > 1.f)
//        //    return float4(1.f, 1.f, 1.f, 1.f);
//        //else
//        //    return float4(0, 0, 0, 1.f);
//    //}
//
//    return color;
//}

