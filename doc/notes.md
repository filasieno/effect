# Typst code generation

- Details:
  - Doxygen does not understand Typst; it won’t style it, but it will carry your text into the XML. However, Doxygen still applies its own parsing (Markdown, commands, autolinks) and XML-escapes characters.
  - If you want true raw Typst blocks in the XML, wrap them in Doxygen verbatim blocks. Those are preserved as-is in XML.
- Minimal processing (good if you’ll parse XML to Typst later):
  - Keep writing normal comments. Doxygen will put the processed text into `build/doc/xml/*.xml`. Be aware of:
    - Markdown transformations (headings, lists)
    - Auto-linking of identifiers
    - XML escaping of `<`, `>`, `&` (you’ll need to unescape in your Typst generator)
- Fully raw Typst snippets (best for embedding ready-to-render Typst):
  - In comments, use:

    ```text
    /// @verbatim
    /// #set text(size: 12pt)
    /// = My API
    /// Some Typst content here…
    /// @endverbatim
    ```

  - Those blocks are emitted verbatim in the XML, so you can extract them and drop straight into Typst.

Optional Doxyfile tweaks to reduce interference:

- Set:
  - MARKDOWN_SUPPORT = NO
  - AUTOLINK_SUPPORT = NO
- Keep:
  - GENERATE_XML = YES
  - XML_PROGRAMLISTING = YES (if you want code listings in XML)

This setup lets you use Doxygen only as a structure extractor while you control the final rendering in Typst.
