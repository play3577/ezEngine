#pragma once

EZ_FORCE_INLINE ezColorLinearUB::ezColorLinearUB(ezUInt8 R, ezUInt8 G, ezUInt8 B, ezUInt8 A /* = 255*/) : ezColorUnsignedByteBase(R, G, B, A)
{
}

inline ezColorLinearUB::ezColorLinearUB(const ezColor& color)
{
  *this = color;
}

inline void ezColorLinearUB::operator=(const ezColor& color)
{
  r = static_cast<ezUInt8>(ezMath::Min(255.0f, ((color.r * 255.0f) + 0.5f)));
  g = static_cast<ezUInt8>(ezMath::Min(255.0f, ((color.g * 255.0f) + 0.5f)));
  b = static_cast<ezUInt8>(ezMath::Min(255.0f, ((color.b * 255.0f) + 0.5f)));
  a = static_cast<ezUInt8>(ezMath::Min(255.0f, ((color.a * 255.0f) + 0.5f)));
}

inline ezColor ezColorLinearUB::ToLinearFloat() const
{
  /// \test this is new

  return ezColor(r * (1.0f / 255.0f), g * (1.0f / 255.0f), b * (1.0f / 255.0f), a * (1.0f / 255.0f));
}

// *****************

EZ_FORCE_INLINE ezColorGammaUB::ezColorGammaUB(ezUInt8 R, ezUInt8 G, ezUInt8 B, ezUInt8 A) : ezColorUnsignedByteBase(R, G, B, A)
{
  /// \test this is new
}

inline ezColorGammaUB::ezColorGammaUB(const ezColor& color)
{
  /// \test this is new

  *this = color;
}

inline void ezColorGammaUB::operator=(const ezColor& color)
{
  /// \test this is new

  const ezVec3 gamma = ezColor::LinearToGamma(ezVec3(color.r, color.g, color.b));

  r = static_cast<ezUInt8>(ezMath::Min(255.0f, ((gamma.x * 255.0f) + 0.5f)));
  g = static_cast<ezUInt8>(ezMath::Min(255.0f, ((gamma.y * 255.0f) + 0.5f)));
  b = static_cast<ezUInt8>(ezMath::Min(255.0f, ((gamma.z * 255.0f) + 0.5f)));
  a = static_cast<ezUInt8>(ezMath::Min(255.0f, ((color.a * 255.0f) + 0.5f)));
}

inline ezColor ezColorGammaUB::ToLinearFloat() const
{
  /// \test this is new

  ezVec3 gamma;
  gamma.x = r * (1.0f / 255.0f);
  gamma.y = g * (1.0f / 255.0f);
  gamma.z = b * (1.0f / 255.0f);

  const ezVec3 linear = ezColor::GammaToLinear(gamma);

  return ezColor(linear.x, linear.y, linear.z, a * (1.0f / 255.0f));
}


