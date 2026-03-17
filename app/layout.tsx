import type { Metadata } from "next";
import "./globals.css";

export const metadata: Metadata = {
  title: "keyBoard - Input Device Manager",
  description: "Input device grabber and remapper",
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en">
      <body>{children}</body>
    </html>
  );
}
