#ifndef OPTIONS_H
#define OPTIONS_H

/*
 * Options for an analyse
 */
class Options
{
public:
    Options();

    //hop level at which connection candidates are searched.
    int candidates_depth=3;
};

#endif // OPTIONS_H
